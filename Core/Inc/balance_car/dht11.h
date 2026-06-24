#ifndef BALANCE_CAR_DHT11_H
#define BALANCE_CAR_DHT11_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct {
    float temperature_c;
    float humidity_percent;
} Dht11Reading_t;

HAL_StatusTypeDef Dht11_Init(void);
HAL_StatusTypeDef Dht11_StartRead(void);
HAL_StatusTypeDef Dht11_FinishRead(Dht11Reading_t *reading);
uint8_t Dht11_GetFailStep(void);

#endif
