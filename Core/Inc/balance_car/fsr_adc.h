#ifndef BALANCE_CAR_FSR_ADC_H
#define BALANCE_CAR_FSR_ADC_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef FsrAdc_Init(void);
HAL_StatusTypeDef FsrAdc_Read(uint16_t *adc_raw, uint16_t *voltage_mv);

#endif
