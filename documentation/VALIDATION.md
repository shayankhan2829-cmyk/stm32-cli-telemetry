# Final validation record

- Corrected CubeMX configuration generated successfully with no clock warning: HCLK 180 MHz; PLLSAIP USB clock 48 MHz.
- CubeIDE final build at 19:47:59 on 3 October 2026: **0 errors, 0 warnings**, duration 21.363 seconds. See [build-console.png](build-console.png).
- Final ELF programmed and verified on NUCLEO-F446ZE, ST-LINK V2J47M34. See [flash-verification.txt](flash-verification.txt).
- Live RCC registers were read through SWD after programming: PLLCFGR=0x27402D04, CFGR=0x0000940A. These encode HSE PLL source, PLLM=4, PLLN=180, PLLP=2, PLL selected as system clock, AHB /1, APB1 /4, APB2 /2. With the board's 8 MHz MCO this gives HCLK 180 MHz. This is register/configuration verification, not an oscilloscope frequency measurement. See [clock-registers.txt](clock-registers.txt).
- After the user closed PuTTY, the final corrected firmware was reset through ST-LINK and captured on COM3 at 115200 8N1. Both exact requested boot lines were received and checked successfully. See [serial-output.txt](serial-output.txt). The serial connection was closed afterward so PuTTY can use COM3.
- Verification was done on the raw UART bytes through the Windows serial API, not by reading a terminal window.

Follow [REPEATABLE_WORKFLOW.md](REPEATABLE_WORKFLOW.md) for the numbered procedure and actual dialogs/errors encountered. The older WORKFLOW.md contains only the initial session notes.

## v1.0.1 CLI and telemetry firmware (25 Oct 2026)

- Built with STM32CubeIDE 2.1.1 / GCC 14.3: 0 errors, 0 warnings. text 24808, data 120, bss 3456 bytes.
- Programmed over SWD with STM32CubeProgrammer, "Download verified successfully".
- Every CLI command exercised over COM3 at 115200 8N1 by a script, output in [cli-session.txt](cli-session.txt):
  help, status, read, read json, led on/off/toggle on all three LEDs, uid, backspace editing,
  unknown command, bad arguments, out-of-range stream period, over-long line, text and JSON streaming,
  software reset and watchdog reset.
- Reset cause reported correctly for NRST pin, software reset and IWDG timeout.
- `tools/telemetry_logger.py` (pyserial 3.5, Python 3.14) logged 10 samples at 200 ms with no
  missing lines: [logger-run.csv](logger-run.csv).
- Measured: VDDA 3313 to 3315 mV, die temperature 35.7 to 36.7 °C at idle.

Fixed during this test: an over-long input line used to be cut off and rejected as an unknown command.
It now reports "line too long". The logger used to skip the first sample, because that line arrives
right after the "> " prompt.

## v1.0.2 button debounce (26 Oct 2026)

- The v1.0.1 debounce accepted the first edge and then ignored changes for 20 ms, so a bouncing
  contact could produce a false press/release pair. v1.0.2 fires an event only after the pin level
  has been stable for 30 ms.
- Build: 0 errors, 0 warnings, text 24844 bytes. Flash verified. Full CLI regression repeated:
  [cli-session.txt](cli-session.txt).
- Button test: this board's B1 switch is broken (PC13 reads 0 constantly, with no noise), so PC13
  (CN11 pin 23) was connected to +3.3 V (CN11 pin 16) with a jumper wire, which is the signal
  B1 produces. 6 level changes gave 6 events, correctly alternating pressed/released, with no
  duplicates despite the wire bouncing: [button-test.txt](button-test.txt).
