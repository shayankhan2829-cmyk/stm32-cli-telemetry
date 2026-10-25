# Start here: STM32 projects and cli_telemetry

**New to STM32? Start with [the complete beginner tutorial](BEGINNER_TUTORIAL.md).** It walks from hardware/tools through code placement, build, flash and testing, with checkpoints and troubleshooting. This page is the shorter overview.

## Is this project complete?

Yes: the requested boot-message firmware is built, flashed and tested on the connected NUCLEO-F446ZE. The final corrected firmware returned both expected lines on COM3. Build: 0 errors, 0 warnings. Flash: verified successfully. ST-LINK firmware: V2J47M34. The UART bytes were captured through the Windows serial API.

This page records the Day 1 bring-up (boot message only). The command line, telemetry and watchdog were added in v1.0.0; see the top-level README.md.

## A reusable workflow for STM32 projects

These are the normal stages for CubeMX/HAL projects. The particular pins, clocks, peripherals, packages and debug connection must be chosen for each board; do not copy this board's numerical settings into every STM32 project.

1. **Identify your hardware and desired behavior.** Write down the board/MCU, power source, debug connector, required inputs/outputs and what success looks like. Check the board manual and schematic for pin routing. A UART name or connector on one board may differ on another.

2. **Install the tools.** Install STM32CubeIDE, the appropriate USB/debug drivers and the required MCU firmware package. With CubeIDE 2.x, use standalone STM32CubeMX to configure hardware and generate the IDE project. A serial terminal is needed only if your firmware uses serial communication. Tool updates and ST-LINK firmware updates are occasional setup work, not steps to repeat every build.

3. **Create a project.** In CubeMX use Board Selector for a supported development board, or MCU Selector for a custom design. Name the project, choose its folder and select STM32CubeIDE as the toolchain. Decide whether to initialize board-default peripherals. Keep each project in a distinct folder.

4. **Configure clocks.** Select an oscillator that physically exists on your hardware. Set the system and peripheral clocks within that MCU's limits. Resolve red/pink clock errors before generation. Do not assume that maximum CPU speed also supplies a valid USB or other peripheral clock.

5. **Configure peripherals and pins.** Enable only the functions your design needs, choose their operating modes and check pin conflicts. Match external devices' voltage levels and settings. For UART, both ends must agree on baud rate, data bits, parity, stop bits and flow control.

6. **Generate code.** Save the .ioc configuration and generate. Accept the necessary firmware package download and license when prompted, and wait for successful generation. The .ioc records the hardware configuration; generated C files implement its initialization.

7. **Import into the IDE.** In CubeIDE 2.x choose File → STM32 Project Create/Import → STM32CubeMX/STM32CubeIDE Project and select the generated folder. After later regenerations, refresh the existing project instead of importing it again.

8. **Write application code.** Keep changes inside USER CODE sections, or in your own source/header files, so regeneration preserves them. Initialize peripherals before using them. Add appropriate error handling and avoid blocking forever in time-sensitive applications.

9. **Build.** Select the project and use Project → Build All/Ctrl+B. Read the console, fix compiler/linker errors and investigate warnings. A successful build proves that the source compiles, not that hardware behavior is correct.

10. **Connect, flash and run.** Attach the board through its debug USB connector. Select the intended project/executable, update the probe firmware if required, then Run or Debug. Verify that programming succeeded. Debug commonly pauses at main; Run should execute the program.

11. **Test the actual behavior.** Observe outputs using the relevant terminal, debugger, LEDs or measurement equipment. Open a terminal before resetting if the program prints only at startup. Compare observed output with the success criteria from step 1.

12. **Save the reproducible project.** Retain your .ioc, application sources, IDE project files and notes about hardware/tool settings. After a hardware/configuration change, regenerate, rebuild, reflash and retest. Routine source changes require only rebuild, reflash and retest.

## Settings specific to this project

| Item | cli_telemetry setting |
|---|---|
| Board | NUCLEO-F446ZE, STM32F446ZETx |
| Project folder | D:\stm_projects\nucleo_f446ze_ready\cli_telemetry |
| CubeIDE workspace | D:\stm_projects\workspace_ready |
| Application source | Core\Src\main.c |
| External clock | HSE bypass, 8 MHz from the powered ST-LINK MCO |
| Main PLL | M=4, N=180, P=2 |
| CPU/AHB clock | 180 MHz |
| APB1 / APB2 | 45 MHz / 90 MHz |
| Board-default USB clock | PLLSAI M=4, N=192, P=8 gives 48 MHz |
| Serial peripheral | USART3, PD8 TX and PD9 RX |
| PC connection | ST-LINK Virtual COM Port, currently COM3 |
| Serial settings | 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control |
| Firmware output | Two text lines once per reset |

## How the program works

1. On reset, the processor executes the startup code stored in flash, prepares C runtime memory and enters main().
2. HAL_Init() initializes the HAL and time base. SystemClock_Config() configures the oscillator/PLL and buses, including Over-Drive for the selected 180 MHz clock.
3. Generated functions initialize GPIO, USART3 and the retained board-default USB peripheral. USART3 is ready at 115200 8N1.
4. setvbuf(stdout, NULL, _IONBF, 0) disables standard-output buffering. printf formats each message. The generated syscall support forwards characters to our __io_putchar(), which sends each byte through HAL_UART_Transmit using huart3.
5. Bytes leave PD8, enter the onboard ST-LINK's UART receiver, and travel over the ST-LINK USB connection to Windows COM3. A serial terminal displays the received characters. PD9 is the return path, but this firmware does not process incoming commands.
6. After the two messages, main() stays in its empty while(1) loop. It sends nothing further until reset or power cycling. HAL_MAX_DELAY makes transmission blocking; it is adequate for this short startup message, not a general asynchronous telemetry design.

The 180 MHz text is a fixed message, not a live frequency measurement. The configured 180 MHz clock was separately checked through generated code and live RCC registers. The USB_OTG_FS peripheral in the main MCU is not how these serial messages reach the PC; the separate ST-LINK chip supplies the virtual COM bridge.

## How you can check it yourself

1. Keep the Nucleo connected through the ST-LINK USB connector. Open Device Manager → Ports (COM & LPT) and find STMicroelectronics STLink Virtual COM Port. It is COM3 on this PC at the time of testing.
2. Open PuTTY. Select Serial, enter COM3 (or the number Device Manager shows), and set Speed to 115200. Under Connection → Serial use 8 data bits, 1 stop bit, parity None and flow control None.
3. Click Open. A blank terminal before reset is normal because the firmware prints only once during startup.
4. Press the Nucleo's black reset button. Expect exactly these two lines:

```text
Boot OK - NUCLEO-F446ZE @ 180 MHz
Day 1 complete. CLI Telemetry project ready.
```

5. Press reset again; the same two lines should appear again. Typing into PuTTY produces no firmware response because command handling has not been implemented.
6. If PuTTY cannot open the port, close other serial applications. If the text is garbled, recheck the serial settings. If there is no output, verify the USB connector/COM number, reset after opening the terminal, and ensure the firmware was flashed successfully. Do not expect continuously repeating output.

## Detailed record and evidence

- [REPEATABLE_WORKFLOW.md](REPEATABLE_WORKFLOW.md): numbered setup record, actual dialogs, failures and fixes, including the CubeIDE 2.x workflow and ST-LINK update-mode step.
- [VALIDATION.md](VALIDATION.md): final verification details and limitations.
- [build-console.png](build-console.png): final build screenshot.
- [flash-verification.txt](flash-verification.txt): final programming and verification log.
- [serial-output.txt](serial-output.txt): both boot lines captured from the final corrected firmware.
- [clock-registers.txt](clock-registers.txt): live RCC register readback.
