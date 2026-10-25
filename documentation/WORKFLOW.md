# cli_telemetry setup record

Date: 3 October 2026. Board: NUCLEO-F446ZE. Target: USART3 boot messages at 180 MHz.

## Official references
- ST CubeIDE 2.x workflow: https://community.st.com/stm32-mcus-60/stm32cubeide-2-0-0-workflow-tutorial-160701
- ST board manual UM1974: https://www.st.com/resource/en/user_manual/dm00244518-stm32-nucleo-144-boards-mb1137-stmicroelectronics.pdf
- CubeMX: https://www.st.com/en/development-tools/stm32cubemx.html

## Numbered workflow and observed results

1. Open STM32CubeIDE 2.1.1. The launcher offered D:\stm_projects\cli_telemetry; click Launch. The C/C++ workbench opened with an existing cli_telemetry project. File > New lacked STM32 Project. ST documentation explains that CubeIDE 2.x no longer bundles CubeMX. Use standalone CubeMX to configure and generate, then import into CubeIDE. No IDE update dialog appeared. A transient automation capture error occurred because the launcher closed; selecting the newly opened workbench resolved it.

2. Launch C:\Users\Shayan\AppData\Local\Programs\STM32CubeMX\STM32CubeMX.exe (existing version 6.17.0). We first checked the official download page, which offered 6.18.1, accepted its license, and reached the MyST/guest download dialog. No new version was downloaded or installed because the existing installation was found. CubeMX initially reported 'Error during Data Refresh: Download has failed, some data may be not updated. Please retry later.' Acknowledge with Enter and select Access to Board Selector. Search Commercial Part Number NUCLEO-F446ZE, select the single board result (mounted STM32F446ZET6), and click Start Project. Answer Yes to 'Initialize all peripherals with their default Mode?' The STM32F446ZETx pinout opens. In Project Manager enter cli_telemetry, parent location D:\stm_projects\nucleo_f446ze_ready, Toolchain/IDE STM32CubeIDE, and keep Generate Under Root checked. C sources and an executable project will be generated. Existing projects were preserved because a fresh directory avoids name/file collisions.
