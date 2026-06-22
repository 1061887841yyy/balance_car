#ifndef BALANCE_CAR_MPU6050_HAL_H
#define BALANCE_CAR_MPU6050_HAL_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} Mpu6050Raw_t;

HAL_StatusTypeDef Mpu6050_Init(void);
HAL_StatusTypeDef Mpu6050_ReadRaw(Mpu6050Raw_t *raw);
uint8_t Mpu6050_GetLastId(void);

#endif
