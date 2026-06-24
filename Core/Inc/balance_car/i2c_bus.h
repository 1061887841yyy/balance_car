#ifndef BALANCE_CAR_I2C_BUS_H
#define BALANCE_CAR_I2C_BUS_H

#include "stm32f1xx_hal.h"

HAL_StatusTypeDef BalanceI2C1_Init(void);
I2C_HandleTypeDef *BalanceI2C1_GetHandle(void);

#endif
