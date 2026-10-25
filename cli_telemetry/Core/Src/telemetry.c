/**
  * @file    telemetry.c
  * @brief   On-chip measurements for STM32F446 using direct register access.
  *
  * ADC1 reads the internal temperature sensor (channel 18) and VREFINT
  * (channel 17). Factory calibration values stored in system memory are used
  * to convert raw counts, so no external parts are required.
  */
#include "telemetry.h"
#include "main.h"

/* Factory calibration (RM0390 / DS10693), measured at VDDA = 3.3 V */
#define VREFINT_CAL   (*(const uint16_t *)0x1FFF7A2AUL)
#define TS_CAL1       (*(const uint16_t *)0x1FFF7A2CUL)  /* 30 degC  */
#define TS_CAL2       (*(const uint16_t *)0x1FFF7A2EUL)  /* 110 degC */
#define CAL_VDDA_MV   3300U

#define ADC_CH_VREFINT 17U
#define ADC_CH_TEMP    18U

static const char *reset_cause = "unknown";

static void capture_reset_cause(void)
{
  uint32_t csr = RCC->CSR;

  if (csr & RCC_CSR_IWDGRSTF)      reset_cause = "watchdog (IWDG)";
  else if (csr & RCC_CSR_WWDGRSTF) reset_cause = "window watchdog";
  else if (csr & RCC_CSR_LPWRRSTF) reset_cause = "low-power";
  else if (csr & RCC_CSR_SFTRSTF)  reset_cause = "software";
  else if (csr & RCC_CSR_PORRSTF)  reset_cause = "power-on";
  else if (csr & RCC_CSR_BORRSTF)  reset_cause = "brown-out";
  else if (csr & RCC_CSR_PINRSTF)  reset_cause = "reset pin";

  RCC->CSR |= RCC_CSR_RMVF;        /* clear flags for the next boot */
}

static void adc_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  (void)RCC->APB2ENR;

  /* ADCCLK = PCLK2 / 4 = 22.5 MHz (max 36 MHz); enable TS + VREFINT */
  ADC123_COMMON->CCR = (ADC123_COMMON->CCR & ~ADC_CCR_ADCPRE) | ADC_CCR_ADCPRE_0
                     | ADC_CCR_TSVREFE;

  /* 480 cycles (~21 us) on both internal channels; sensor needs >= 10 us */
  ADC1->SMPR1 |= ADC_SMPR1_SMP17 | ADC_SMPR1_SMP18;
  ADC1->CR1 = 0;                   /* 12-bit, no scan */
  ADC1->SQR1 = 0;                  /* one conversion */
  ADC1->CR2 = ADC_CR2_ADON;

  HAL_Delay(1);                    /* ADC + temp sensor start-up (< 10 us) */
}

static uint16_t adc_read(uint32_t channel)
{
  ADC1->SQR3 = channel;
  ADC1->SR = 0;
  ADC1->CR2 |= ADC_CR2_SWSTART;
  while ((ADC1->SR & ADC_SR_EOC) == 0) { }
  return (uint16_t)ADC1->DR;
}

static uint16_t adc_read_avg(uint32_t channel)
{
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++)
  {
    sum += adc_read(channel);
  }
  return (uint16_t)(sum / 8U);
}

void telemetry_init(void)
{
  capture_reset_cause();
  adc_init();
}

void telemetry_read(telemetry_sample_t *s)
{
  uint32_t vref_raw = adc_read_avg(ADC_CH_VREFINT);
  uint32_t ts_raw   = adc_read_avg(ADC_CH_TEMP);

  s->uptime_ms = HAL_GetTick();
  s->vdda_mv   = vref_raw ? (CAL_VDDA_MV * VREFINT_CAL) / vref_raw : 0;

  /* Rescale the sensor reading to the 3.3 V calibration reference */
  int32_t ts_cal = (int32_t)((ts_raw * s->vdda_mv) / CAL_VDDA_MV);
  s->temp_centi_c = ((ts_cal - (int32_t)TS_CAL1) * (110 - 30) * 100)
                    / ((int32_t)TS_CAL2 - (int32_t)TS_CAL1) + 30 * 100;

  s->button = (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET);
}

const char *telemetry_reset_cause(void)
{
  return reset_cause;
}

void telemetry_uid(uint32_t uid[3])
{
  const uint32_t *p = (const uint32_t *)UID_BASE;
  uid[0] = p[0];
  uid[1] = p[1];
  uid[2] = p[2];
}

void watchdog_start(void)
{
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;  /* pause while halted */

  IWDG->KR  = 0xCCCC;              /* start (also enables LSI) */
  IWDG->KR  = 0x5555;              /* unlock PR/RLR */
  IWDG->PR  = 4;                   /* LSI 32 kHz / 64 = 500 Hz */
  IWDG->RLR = 1000;                /* 1000 / 500 Hz = 2 s */
  while (IWDG->SR != 0) { }
  IWDG->KR  = 0xAAAA;
}

void watchdog_kick(void)
{
  IWDG->KR = 0xAAAA;
}
