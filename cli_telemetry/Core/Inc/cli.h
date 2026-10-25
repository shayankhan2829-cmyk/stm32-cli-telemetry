/**
  * @file    cli.h
  * @brief   Interrupt-driven UART command line with periodic telemetry stream.
  */
#ifndef CLI_H
#define CLI_H

#include "main.h"

#define FW_VERSION "1.0.2"

void cli_init(UART_HandleTypeDef *huart);
void cli_poll(void);      /* call from the main loop; never blocks on input */
void cli_rx_isr(void);    /* call from USARTx_IRQHandler */

#endif /* CLI_H */
