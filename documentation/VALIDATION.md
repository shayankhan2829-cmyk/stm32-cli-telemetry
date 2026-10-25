# Final validation record

- Corrected CubeMX configuration generated successfully with no clock warning: HCLK 180 MHz; PLLSAIP USB clock 48 MHz.
- CubeIDE final build at 19:47:59 on 3 October 2026: **0 errors, 0 warnings**, duration 21.363 seconds. See [build-console.png](build-console.png).
- Final ELF programmed and verified on NUCLEO-F446ZE, ST-LINK V2J47M34. See [flash-verification.txt](flash-verification.txt).
- Live RCC registers were read through SWD after programming: PLLCFGR=0x27402D04, CFGR=0x0000940A. These encode HSE PLL source, PLLM=4, PLLN=180, PLLP=2, PLL selected as system clock, AHB /1, APB1 /4, APB2 /2. With the board's 8 MHz MCO this gives HCLK 180 MHz. This is register/configuration verification, not an oscilloscope frequency measurement. See [clock-registers.txt](clock-registers.txt).
- After the user closed PuTTY, the final corrected firmware was reset through ST-LINK and captured on COM3 at 115200 8N1. Both exact requested boot lines were received and checked successfully. See [serial-output.txt](serial-output.txt). The serial connection was closed afterward so PuTTY can use COM3.
- Verification was done on the raw UART bytes through the Windows serial API, not by reading a terminal window.

Follow [REPEATABLE_WORKFLOW.md](REPEATABLE_WORKFLOW.md) for the numbered procedure and actual dialogs/errors encountered. The older WORKFLOW.md contains only the initial session notes.
