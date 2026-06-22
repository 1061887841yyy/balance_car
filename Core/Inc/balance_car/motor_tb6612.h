#ifndef BALANCE_CAR_MOTOR_TB6612_H
#define BALANCE_CAR_MOTOR_TB6612_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define MOTOR_PWM_LIMIT 100

HAL_StatusTypeDef TB6612_Init(void);
void TB6612_SetMotors(int16_t left_pwm, int16_t right_pwm);
void TB6612_Stop(void);

#endif
