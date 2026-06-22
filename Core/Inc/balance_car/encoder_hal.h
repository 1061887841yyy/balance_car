#ifndef BALANCE_CAR_ENCODER_HAL_H
#define BALANCE_CAR_ENCODER_HAL_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef Encoder_Init(void);
int16_t Encoder_GetLeftDelta(void);
int16_t Encoder_GetRightDelta(void);

#endif
