/**
  * @file    cli.c
  * @brief   Interrupt-driven UART command line with periodic telemetry stream.
  *
  * RX: the USART RXNE interrupt pushes bytes into a lock-free ring buffer
  *     (single producer ISR, single consumer main loop).
  * TX: printf -> __io_putchar -> HAL_UART_Transmit (blocking, ~87 us/byte).
  */
#include "cli.h"
#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RX_BUF_SIZE  128U          /* power of two */
#define LINE_MAX     64U
#define MAX_ARGS     4U
#define PROMPT       "> "

static UART_HandleTypeDef *cli_uart;

static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head;  /* written by ISR */
static volatile uint16_t rx_tail;  /* written by main loop */
static volatile uint32_t rx_overruns;

static char     line[LINE_MAX];
static uint16_t line_len;
static uint8_t  line_overflow;

static uint32_t stream_period_ms;  /* 0 = stream off */
static uint32_t stream_last_ms;
static uint8_t  stream_json;

#define BTN_STABLE_MS 30U

static uint8_t  btn_state;         /* debounced */
static uint8_t  btn_raw_last;
static uint32_t btn_raw_ms;        /* when the raw level last changed */

/* ---- commands ----------------------------------------------------------- */

typedef void (*cmd_fn)(int argc, char *argv[]);

typedef struct
{
  const char *name;
  const char *usage;
  cmd_fn      fn;
} cmd_t;

static void cmd_help(int argc, char *argv[]);
static void cmd_status(int argc, char *argv[]);
static void cmd_read(int argc, char *argv[]);
static void cmd_stream(int argc, char *argv[]);
static void cmd_led(int argc, char *argv[]);
static void cmd_uid(int argc, char *argv[]);
static void cmd_reset(int argc, char *argv[]);
static void cmd_wdtest(int argc, char *argv[]);

static const cmd_t commands[] =
{
  { "help",   "help                      list commands",                    cmd_help   },
  { "status", "status                    firmware, clocks, uptime, reset",  cmd_status },
  { "read",   "read                      one telemetry sample",             cmd_read   },
  { "stream", "stream <ms|off> [json]    periodic telemetry (50..60000 ms)", cmd_stream },
  { "led",    "led <1|2|3> <on|off|toggle>",                                cmd_led    },
  { "uid",    "uid                       96-bit unique device ID",          cmd_uid    },
  { "reset",  "reset                     software reset",                   cmd_reset  },
  { "wdtest", "wdtest                    stop kicking watchdog (resets ~2 s)", cmd_wdtest },
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void print_sample(const telemetry_sample_t *s, uint8_t json)
{
  int32_t t = s->temp_centi_c;
  const char *sign = (t < 0) ? "-" : "";
  if (t < 0) t = -t;

  if (json)
  {
    printf("{\"t_ms\":%lu,\"temp_c\":%s%ld.%02ld,\"vdda_mv\":%lu,\"btn\":%u}\r\n",
           (unsigned long)s->uptime_ms, sign, (long)(t / 100), (long)(t % 100),
           (unsigned long)s->vdda_mv, s->button);
  }
  else
  {
    printf("t_ms=%lu temp_c=%s%ld.%02ld vdda_mv=%lu btn=%u\r\n",
           (unsigned long)s->uptime_ms, sign, (long)(t / 100), (long)(t % 100),
           (unsigned long)s->vdda_mv, s->button);
  }
}

static void cmd_help(int argc, char *argv[])
{
  (void)argc; (void)argv;
  for (unsigned i = 0; i < NUM_COMMANDS; i++)
  {
    printf("  %s\r\n", commands[i].usage);
  }
}

static void cmd_status(int argc, char *argv[])
{
  (void)argc; (void)argv;
  uint32_t up = HAL_GetTick() / 1000U;

  printf("firmware   : cli_telemetry v%s (%s %s)\r\n", FW_VERSION, __DATE__, __TIME__);
  printf("mcu        : STM32F446ZE rev 0x%04lx\r\n", (unsigned long)(DBGMCU->IDCODE >> 16));
  printf("sysclk     : %lu Hz\r\n", (unsigned long)HAL_RCC_GetSysClockFreq());
  printf("hclk/pclk1/pclk2 : %lu / %lu / %lu Hz\r\n",
         (unsigned long)HAL_RCC_GetHCLKFreq(),
         (unsigned long)HAL_RCC_GetPCLK1Freq(),
         (unsigned long)HAL_RCC_GetPCLK2Freq());
  printf("uptime     : %luh %02lum %02lus\r\n",
         (unsigned long)(up / 3600U), (unsigned long)((up / 60U) % 60U), (unsigned long)(up % 60U));
  printf("reset cause: %s\r\n", telemetry_reset_cause());
  printf("rx overruns: %lu\r\n", (unsigned long)rx_overruns);
  printf("stream     : %s", stream_period_ms ? "" : "off\r\n");
  if (stream_period_ms)
  {
    printf("%lu ms (%s)\r\n", (unsigned long)stream_period_ms, stream_json ? "json" : "text");
  }
}

static void cmd_read(int argc, char *argv[])
{
  telemetry_sample_t s;
  telemetry_read(&s);
  print_sample(&s, (argc > 1 && strcmp(argv[1], "json") == 0));
}

static void cmd_stream(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("usage: stream <ms|off> [json]\r\n");
    return;
  }
  if (strcmp(argv[1], "off") == 0)
  {
    stream_period_ms = 0;
    printf("stream off\r\n");
    return;
  }

  char *end;
  long ms = strtol(argv[1], &end, 10);
  if (*end != '\0' || ms < 50 || ms > 60000)
  {
    printf("error: period must be 50..60000 ms\r\n");
    return;
  }
  stream_json      = (argc > 2 && strcmp(argv[2], "json") == 0);
  stream_period_ms = (uint32_t)ms;
  stream_last_ms   = HAL_GetTick() - stream_period_ms;  /* first sample now */
  printf("stream every %ld ms (%s), 'stream off' to stop\r\n", ms, stream_json ? "json" : "text");
}

static void cmd_led(int argc, char *argv[])
{
  static const uint16_t pins[] = { LD1_Pin, LD2_Pin, LD3_Pin };
  static const char *names[]   = { "green", "blue", "red" };

  if (argc < 3 || argv[1][0] < '1' || argv[1][0] > '3' || argv[1][1] != '\0')
  {
    printf("usage: led <1|2|3> <on|off|toggle>\r\n");
    return;
  }
  int idx = argv[1][0] - '1';

  if (strcmp(argv[2], "on") == 0)          HAL_GPIO_WritePin(GPIOB, pins[idx], GPIO_PIN_SET);
  else if (strcmp(argv[2], "off") == 0)    HAL_GPIO_WritePin(GPIOB, pins[idx], GPIO_PIN_RESET);
  else if (strcmp(argv[2], "toggle") == 0) HAL_GPIO_TogglePin(GPIOB, pins[idx]);
  else
  {
    printf("usage: led <1|2|3> <on|off|toggle>\r\n");
    return;
  }
  printf("led%d (%s) = %s\r\n", idx + 1, names[idx],
         HAL_GPIO_ReadPin(GPIOB, pins[idx]) == GPIO_PIN_SET ? "on" : "off");
}

static void cmd_uid(int argc, char *argv[])
{
  (void)argc; (void)argv;
  uint32_t uid[3];
  telemetry_uid(uid);
  printf("%08lX-%08lX-%08lX\r\n", (unsigned long)uid[2], (unsigned long)uid[1], (unsigned long)uid[0]);
}

static void cmd_reset(int argc, char *argv[])
{
  (void)argc; (void)argv;
  printf("resetting...\r\n");
  HAL_Delay(5);
  NVIC_SystemReset();
}

static void cmd_wdtest(int argc, char *argv[])
{
  (void)argc; (void)argv;
  printf("simulating a hang; IWDG should reset the MCU in ~2 s\r\n");
  __disable_irq();
  while (1) { }
}

/* ---- line handling ------------------------------------------------------ */

static void execute_line(char *buf)
{
  char *argv[MAX_ARGS];
  int argc = 0;

  for (char *tok = strtok(buf, " \t"); tok && argc < (int)MAX_ARGS; tok = strtok(NULL, " \t"))
  {
    argv[argc++] = tok;
  }
  if (argc == 0)
  {
    return;
  }

  for (unsigned i = 0; i < NUM_COMMANDS; i++)
  {
    if (strcmp(argv[0], commands[i].name) == 0)
    {
      commands[i].fn(argc, argv);
      return;
    }
  }
  printf("unknown command '%s', type 'help'\r\n", argv[0]);
}

static int rx_pop(uint8_t *c)
{
  uint16_t tail = rx_tail;
  if (tail == rx_head)
  {
    return 0;
  }
  *c = rx_buf[tail];
  rx_tail = (uint16_t)((tail + 1U) & (RX_BUF_SIZE - 1U));
  return 1;
}

static void handle_char(uint8_t c)
{
  if (c == '\r' || c == '\n')
  {
    printf("\r\n");
    line[line_len] = '\0';
    if (line_overflow)
    {
      printf("error: line too long (max %u chars)\r\n", (unsigned)(LINE_MAX - 1U));
    }
    else
    {
      execute_line(line);
    }
    line_len = 0;
    line_overflow = 0;
    printf(PROMPT);
  }
  else if (c == '\b' || c == 0x7F)
  {
    if (line_len > 0)
    {
      line_len--;
      printf("\b \b");
    }
  }
  else if (c >= 0x20 && c < 0x7F)
  {
    if (line_len < LINE_MAX - 1U)
    {
      line[line_len++] = (char)c;
      putchar(c);
    }
    else
    {
      line_overflow = 1;           /* keep swallowing until Enter */
    }
  }
}

/* ---- public API --------------------------------------------------------- */

void cli_init(UART_HandleTypeDef *huart)
{
  cli_uart = huart;
  btn_state = (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET);
  btn_raw_last = btn_state;

  __HAL_UART_ENABLE_IT(cli_uart, UART_IT_RXNE);
  HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);

  printf("\r\n=== cli_telemetry v%s | NUCLEO-F446ZE @ %lu MHz ===\r\n",
         FW_VERSION, (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000U));
  printf("reset cause: %s\r\n", telemetry_reset_cause());
  printf("type 'help' for commands\r\n" PROMPT);
}

void cli_poll(void)
{
  uint8_t c;
  uint32_t now = HAL_GetTick();

  while (rx_pop(&c))
  {
    handle_char(c);
  }

  if (stream_period_ms && (now - stream_last_ms) >= stream_period_ms)
  {
    telemetry_sample_t s;
    stream_last_ms += stream_period_ms;
    telemetry_read(&s);
    print_sample(&s, stream_json);
  }

  /* Button event once the level has been stable for BTN_STABLE_MS */
  uint8_t raw = (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET);
  if (raw != btn_raw_last)
  {
    btn_raw_last = raw;
    btn_raw_ms = now;
  }
  else if (raw != btn_state && (now - btn_raw_ms) >= BTN_STABLE_MS)
  {
    btn_state = raw;
    printf("\r\nevent: button %s\r\n" PROMPT, btn_state ? "pressed" : "released");
  }
}

void cli_rx_isr(void)
{
  USART_TypeDef *u = cli_uart->Instance;
  uint32_t sr = u->SR;

  if (sr & (USART_SR_RXNE | USART_SR_ORE))
  {
    uint8_t c = (uint8_t)u->DR;    /* reading DR also clears ORE */
    if (sr & USART_SR_ORE)
    {
      rx_overruns++;
    }

    uint16_t next = (uint16_t)((rx_head + 1U) & (RX_BUF_SIZE - 1U));
    if (next != rx_tail)
    {
      rx_buf[rx_head] = c;
      rx_head = next;
    }
    else
    {
      rx_overruns++;               /* ring full, byte dropped */
    }
  }
}
