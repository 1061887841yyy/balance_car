#include "balance_car/motor_tb6612.h"

#define TB6612_STBY_PORT GPIOA
#define TB6612_STBY_PIN  GPIO_PIN_3
#define TB6612_AIN1_PORT GPIOA
#define TB6612_AIN1_PIN  GPIO_PIN_4
#define TB6612_AIN2_PORT GPIOA
#define TB6612_AIN2_PIN  GPIO_PIN_5
#define TB6612_BIN1_PORT GPIOB
#define TB6612_BIN1_PIN  GPIO_PIN_1
#define TB6612_BIN2_PORT GPIOB
#define TB6612_BIN2_PIN  GPIO_PIN_0

#define PWM_PERIOD_COUNTS 1000U

static TIM_HandleTypeDef s_htim3;

static int16_t Motor_Clamp(int16_t pwm)
{
    if (pwm > MOTOR_PWM_LIMIT) {
        return MOTOR_PWM_LIMIT;
    }
    if (pwm < -MOTOR_PWM_LIMIT) {
        return -MOTOR_PWM_LIMIT;
    }
    return pwm;
}

static uint32_t Motor_ToCompare(int16_t pwm)
{
    int16_t abs_pwm = pwm < 0 ? -pwm : pwm;
    return (uint32_t)abs_pwm * PWM_PERIOD_COUNTS / MOTOR_PWM_LIMIT;
}

HAL_StatusTypeDef TB6612_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = TB6612_STBY_PIN | TB6612_AIN1_PIN | TB6612_AIN2_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = TB6612_BIN1_PIN | TB6612_BIN2_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    s_htim3.Instance = TIM3;
    s_htim3.Init.Prescaler = 64U - 1U;
    s_htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_htim3.Init.Period = PWM_PERIOD_COUNTS;
    s_htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&s_htim3) != HAL_OK) {
        return HAL_ERROR;
    }

    TIM_OC_InitTypeDef config = {0};
    config.OCMode = TIM_OCMODE_PWM1;
    config.Pulse = 0U;
    config.OCPolarity = TIM_OCPOLARITY_HIGH;
    config.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&s_htim3, &config, TIM_CHANNEL_1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_ConfigChannel(&s_htim3, &config, TIM_CHANNEL_2) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_Start(&s_htim3, TIM_CHANNEL_1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_Start(&s_htim3, TIM_CHANNEL_2) != HAL_OK) {
        return HAL_ERROR;
    }

    TB6612_Stop();
    return HAL_OK;
}

void TB6612_SetMotors(int16_t left_pwm, int16_t right_pwm)
{
    left_pwm = Motor_Clamp(left_pwm);
    right_pwm = Motor_Clamp(right_pwm);

    HAL_GPIO_WritePin(TB6612_STBY_PORT, TB6612_STBY_PIN, GPIO_PIN_SET);

    if (left_pwm >= 0) {
        HAL_GPIO_WritePin(TB6612_AIN1_PORT, TB6612_AIN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(TB6612_AIN2_PORT, TB6612_AIN2_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(TB6612_AIN1_PORT, TB6612_AIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(TB6612_AIN2_PORT, TB6612_AIN2_PIN, GPIO_PIN_SET);
    }

    if (right_pwm >= 0) {
        HAL_GPIO_WritePin(TB6612_BIN1_PORT, TB6612_BIN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(TB6612_BIN2_PORT, TB6612_BIN2_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(TB6612_BIN1_PORT, TB6612_BIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(TB6612_BIN2_PORT, TB6612_BIN2_PIN, GPIO_PIN_SET);
    }

    __HAL_TIM_SET_COMPARE(&s_htim3, TIM_CHANNEL_1, Motor_ToCompare(left_pwm));
    __HAL_TIM_SET_COMPARE(&s_htim3, TIM_CHANNEL_2, Motor_ToCompare(right_pwm));
}

void TB6612_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&s_htim3, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COMPARE(&s_htim3, TIM_CHANNEL_2, 0U);
    HAL_GPIO_WritePin(TB6612_AIN1_PORT, TB6612_AIN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TB6612_AIN2_PORT, TB6612_AIN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TB6612_BIN1_PORT, TB6612_BIN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TB6612_BIN2_PORT, TB6612_BIN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TB6612_STBY_PORT, TB6612_STBY_PIN, GPIO_PIN_RESET);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim_pwm)
{
    GPIO_InitTypeDef gpio = {0};

    if (htim_pwm->Instance != TIM3) {
        return;
    }

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}
