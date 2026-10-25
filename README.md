# cli_telemetry — STM32F446 UART command line + live telemetry

Bare-metal firmware for the **NUCLEO-F446ZE** (Cortex-M4F @ 180 MHz). It gives a
serial command line over the ST-LINK virtual COM port, on-chip telemetry
(die temperature and VDDA from factory calibration), LED/button I/O, reset-cause
reporting and an independent watchdog. A Python host tool logs the stream to CSV.

Built with STM32CubeMX + STM32CubeIDE 2.1 (GCC 14.3, HAL). No external
hardware needed: plug in the board and open a terminal.

## Features

| Area | What it does | How |
|---|---|---|
| Clock | 180 MHz SYSCLK from 8 MHz HSE bypass, PLL M/N/P = 4/180/2, Over-Drive | CubeMX, verified via live RCC register readback |
| UART RX | Non-blocking, interrupt-driven | USART3 RXNE ISR → 128-byte lock-free ring buffer, overrun counter |
| CLI | Line editing (backspace), tokenizer, command table, argument validation | `Core/Src/cli.c` |
| Telemetry | Die temperature (0.01 °C), VDDA (mV), button state, uptime | ADC1 ch17/ch18 at register level, 8-sample average, TS_CAL1/TS_CAL2/VREFINT_CAL |
| Streaming | Periodic samples, text or JSON, 50 ms – 60 s | drift-free scheduler on `HAL_GetTick()` |
| Reliability | 2 s independent watchdog, reset-cause on boot, frozen while debugging | IWDG + RCC->CSR + DBGMCU |
| Events | Debounced USER button press/release messages | 20 ms debounce |
| Host tool | Serial → CSV logger | `tools/telemetry_logger.py` (pyserial) |

## Commands (115200 8N1, ST-LINK VCP)

```text
help                      list commands
status                    firmware, clocks, uptime, reset
read [json]               one telemetry sample
stream <ms|off> [json]    periodic telemetry (50..60000 ms)
led <1|2|3> <on|off|toggle>
uid                       96-bit unique device ID
reset                     software reset
wdtest                    stop kicking watchdog (resets ~2 s)
```

JSON stream format:

```json
{"t_ms":12500,"temp_c":31.42,"vdda_mv":3291,"btn":0}
```

## Layout

```text
cli_telemetry/
  cli_telemetry.ioc          CubeMX hardware configuration
  Core/Src/main.c            init sequence + super-loop (USER CODE sections only)
  Core/Src/cli.c, cli.h      ring buffer, line editor, command table, stream
  Core/Src/telemetry.c, .h   ADC temp/VDDA, UID, reset cause, IWDG
  Core/Src/stm32f4xx_it.c    USART3_IRQHandler -> cli_rx_isr()
tools/telemetry_logger.py    host-side CSV logger
documentation/               bring-up record, build/flash/clock evidence
```

## Build and flash

1. Open STM32CubeIDE, *File → Import → Existing Projects*, select `cli_telemetry/`.
2. *Project → Build All* (expected: 0 errors, 0 warnings, ~25 KB flash).
3. Connect the Nucleo's ST-LINK USB, *Run*. Or from the command line:

```bash
STM32_Programmer_CLI -c port=SWD -w cli_telemetry/Debug/cli_telemetry.elf -v -rst
```

4. Open the COM port at 115200 8N1 (PuTTY, Tera Term, `python -m serial.tools.miniterm COM3 115200`), press reset, type `help`.

## Log telemetry on the PC

```bash
pip install pyserial
python tools/telemetry_logger.py COM3 --period 500 --out log.csv
```

## Design notes

- **Why register-level ADC?** Only two internal channels are needed; avoiding the HAL ADC module keeps the code small and shows exactly what the hardware does. Calibration addresses are from the STM32F446 datasheet.
- **Integer math only.** newlib-nano `printf` has no float support by default, so temperatures are computed and printed as fixed-point centi-degrees.
- **Regeneration-safe.** All edits to CubeMX files are inside `USER CODE` blocks; application logic lives in separate files. If USART3 global interrupt is later enabled in CubeMX, remove the generated `USART3_IRQHandler` or route it to `cli_rx_isr()`.
- **TX is blocking** (~87 µs/byte at 115200). Fine for a console; the next step for high-rate logging is DMA TX with a double buffer.

## Status

- Clock tree, build, flash and UART output verified on hardware ([documentation/VALIDATION.md](documentation/VALIDATION.md)).
- v1.0.0 CLI/telemetry firmware: builds with 0 errors / 0 warnings in STM32CubeIDE 2.1.1. Hardware session log: see `documentation/cli-session.txt`.
