/**
  * @file    telemetry.h
  * @brief   On-chip measurements: die temperature, VDDA, unique ID, reset cause,
  *          independent watchdog. Register-level, no extra HAL modules needed.
  */
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

typedef struct
{
  uint32_t uptime_ms;
  int32_t  temp_centi_c;   /* die temperature, 0.01 degC */
  uint32_t vdda_mv;        /* analog supply, derived from VREFINT */
  uint8_t  button;         /* blue USER button, 1 = pressed */
} telemetry_sample_t;

void        telemetry_init(void);
void        telemetry_read(telemetry_sample_t *s);
const char *telemetry_reset_cause(void);
void        telemetry_uid(uint32_t uid[3]);

void        watchdog_start(void);  /* ~2 s timeout */
void        watchdog_kick(void);

#endif /* TELEMETRY_H */
