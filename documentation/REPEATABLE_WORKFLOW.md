# cli_telemetry — NUCLEO-F446ZE workflow

Project: `D:\stm_projects\nucleo_f446ze_ready\cli_telemetry`  
CubeIDE workspace: `D:\stm_projects\workspace_ready`  
Serial connection observed: **COM3, 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control**.

1. **Open the development tools.** Launch STM32CubeIDE 2.1.1 and choose the workspace above. This version's File menu provides “STM32 Project Create/Import”; it does not contain the integrated CubeMX board wizard used by older tutorials. We therefore used standalone STM32CubeMX from `C:\Users\Shayan\AppData\Local\Programs\STM32CubeMX\STM32CubeMX.exe`. Installing a newer version does not automatically move a project: open its `.ioc` in CubeMX and import the generated directory in CubeIDE. Existing projects were retained in their original locations.

2. **Select the board.** In CubeMX open Board Selector, search `NUCLEO-F446ZE`, select the board carrying STM32F446ZET6 (project device STM32F446ZETx), and start the project. Choose Yes when asked to initialize peripherals with their default modes. The board pinout appeared. An earlier database refresh download error was acknowledged; the local board definition remained available. In Project Manager use project name `cli_telemetry`, parent folder `D:\stm_projects\nucleo_f446ze_ready`, and Toolchain/IDE `STM32CubeIDE`, with Generate Under Root selected. The generated sources are C and the project builds an executable.

3. **Configure the 180 MHz system clock.** Open Clock Configuration and set HCLK to 180 MHz. The automatic solver stalled during the original attempt, so the configuration was completed manually: HSE bypass at 8 MHz from ST-LINK MCO; main PLL source HSE; PLLM=4, PLLN=180, PLLP=2; SYSCLK source PLLCLK; AHB divide by 1; APB1 divide by 4; APB2 divide by 2. This gives SYSCLK/HCLK 180 MHz, PCLK1 45 MHz and PCLK2 90 MHz. Generated code enables voltage scale 1 and Over-Drive and uses flash latency 5. Keep ST-LINK powered because this clock configuration uses its MCO signal.

4. **Resolve the board-default USB clock.** Initial board defaults enabled USB_OTG_FS, and the main PLLQ=7 produced 51.428571 MHz, shown in pink. On the clock diagram choose PLLSAIP in the PLL48CLK mux, set PLLSAIM=4, PLLSAIN=192 and PLLSAIP=8. This produces exactly 48 MHz from the 8 MHz HSE. After applying these values the pink error fields and the clock-error indicator disappeared; Resolve Clock Issues became disabled. This USB clock is independent of the ST-LINK virtual COM bridge used for this task.

5. **Verify USART3.** Under Pinout & Configuration → Connectivity → USART3 choose Asynchronous. Verify PD8=USART3_TX and PD9=USART3_RX, connected to ST-LINK's receive/transmit signals. Use baud 115200, word length 8 bits, one stop bit, no parity, TX/RX mode, and no hardware flow control. These assignments were confirmed in the saved `.ioc` and generated HAL initialization. Do not substitute USART2.

6. **Generate sources and install firmware packages.** Click Generate Code. When CubeMX reported missing STM32Cube FW_F4 V1.28.3/dependencies, accept the download. It downloaded the 1.28.0 base archive and 1.28.3 update archive and displayed a license agreement for each; select acceptance and Finish. Wait for both extraction operations. The original generation displayed a clock warning, which was corrected in step 4 before final regeneration. Successful generation reports the project directory and offers Open Folder, Open Project and Close. A Windows firewall prompt appeared during the original attempt; it was no longer present on the next successful continuation.

7. **Add the boot messages in regeneration-safe sections.** In `Core\Src\main.c`, put `#include <stdio.h>` between USER CODE BEGIN/END Includes. Put the `__io_putchar` function below between USER CODE BEGIN/END 0, after the generated `huart3` declaration. Put `setvbuf` and both `printf` calls between USER CODE BEGIN/END 2, after generated peripheral initialization (including `MX_USART3_UART_Init()`). We initially placed stdio outside the user section and moved it inside before final regeneration so CubeMX preserves it. Use the exact code below.

```c
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END 0 */

/* Inside main(), USER CODE BEGIN 2: */
setvbuf(stdout, NULL, _IONBF, 0);
printf("Boot OK - NUCLEO-F446ZE @ 180 MHz\r\n");
printf("Day 1 complete. CLI Telemetry project ready.\r\n");
```

8. **Import into CubeIDE.** Choose File → STM32 Project Create/Import → Import STM32 Project → STM32CubeMX/STM32CubeIDE Project, then Next. Enter the generated project folder above as Import source. The importer showed one checked project, `cli_telemetry`, with STM32CubeIDE project type; choose Finish. Project Explorer then contained Core, Drivers, Debug and linker scripts. If the project is already imported, select it and Refresh (F5) after external generation instead of importing it twice.

9. **Build.** Select `cli_telemetry` in Project Explorer and choose Project → Build All (Ctrl+B). Wait for completion in the CDT Build Console. The initial build finished with 0 errors and 0 warnings. Clicking Run before selecting the project initially produced “The selection cannot be launched, and there are no recent launches”; selecting the project fixed this. Build evidence is stored alongside this document when available.

10. **Connect and identify the board.** Connect the PC to the Nucleo's ST-LINK USB connector using a data-capable cable. Windows initially exposed no ST-LINK device, and CubeIDE showed “No ST-LINK detected.” After the board was connected, Windows reported ST-Link Debug with OK status and STMicroelectronics STLink Virtual COM Port (COM3). For a manual repeat, open Device Manager → Ports (COM & LPT) and read the COM number there; it can change on another PC/USB port. In this session the same Windows device inventory was read through PowerShell rather than the Device Manager GUI.

11. **Update ST-LINK correctly.** Click CubeIDE Run and accept its firmware-update offer. In STLinkUpgrade 3.17.10 select ST-LINK/V2-1, then click **Open in update mode**. Before this action the version fields say Unknown and Upgrade is disabled; that is not proof of a driver failure. After entering update mode the version was readable and Upgrade became available. Click Upgrade and wait. We observed V2J47M34 and “Upgrade successful.” Close the updater to release the probe. Earlier attempts refreshed the device list without entering update mode and therefore did not update it; declining the mandatory update caused “Failed to start GDB server.” An attempted STLinkServer MSI installation did not establish that a driver repair was needed; the successful fix was the firmware update.

12. **Flash and run.** Select the project and use CubeIDE's green Run button. On first use, the launch configuration showed project `cli_telemetry` and executable `Debug/cli_telemetry.elf`; confirm OK. The saved launch file is in the project. After the firmware update, the Run session progressed through building and debugger connection and later terminated with Exit. Independently, the bundled STM32CubeProgrammer CLI connected under reset, identified NUCLEO-F446ZE/STM32F446xx, programmed the ELF at 0x08000000, reported “Download verified successfully,” and reset the MCU. The first CLI attempt while the updater was still open returned DEV_CONNECT_ERR; closing the updater and connecting under reset resolved it.

13. **Open a serial terminal and reset.** In PuTTY choose Serial, Serial line COM3, Speed 115200. Under Connection → Serial ensure 8 data bits, 1 stop bit, parity None and flow control None, then Open. Press the board's black reset button after the terminal is open because the messages print only once per boot. We captured both exact lines through the Windows serial API at these settings after issuing a software reset through ST-LINK. The UART bytes themselves were verified; the reset was issued through ST-LINK rather than the button.

```text
Boot OK - NUCLEO-F446ZE @ 180 MHz
Day 1 complete. CLI Telemetry project ready.
```

14. **Repeat later.** Open the same CubeIDE workspace, select the project, edit only USER CODE blocks, build and Run. Keep the terminal open for the reset caused by Run, or press reset afterward. If COM3 is busy, close other applications holding that serial port. To regenerate, open the project's `.ioc` in standalone CubeMX, retain the clock and USART3 settings above, generate, then refresh and rebuild in CubeIDE.

Official references: [ST CubeIDE 2.x workflow](https://community.st.com/stm32-mcus-60/stm32cubeide-2-0-0-workflow-tutorial-160701), [NUCLEO-144 user manual UM1974](https://www.st.com/resource/en/user_manual/dm00244518-stm32-nucleo-144-boards-mb1137-stmicroelectronics.pdf), [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html).
