#include "balance_car/encoder_hal.h"

static TIM_HandleTypeDef s_htim1;
static TIM_HandleTypeDef s_htim2;

static HAL_StatusTypeDef Encoder_InitTimer(TIM_HandleTypeDef *htim, TIM_TypeDef *instance)
{
    TIM_Encoder_InitTypeDef config = {0};
    TIM_MasterConfigTypeDef master = {0};

    htim->Instance = instance;
    htim->Init.Prescaler = 0U;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = 65535U;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    config.EncoderMode = TIM_ENCODERMODE_TI12;
    config.IC1Polarity = TIM_ICPOLARITY_RISING;
    config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    config.IC1Prescaler = TIM_ICPSC_DIV1;
    config.IC1Filter = 0x0FU;
    config.IC2Polarity = TIM_ICPOLARITY_RISING;
    config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    config.IC2Prescaler = TIM_ICPSC_DIV1;
    config.IC2Filter = 0x0FU;

    if (HAL_TIM_Encoder_Init(htim, &config) != HAL_OK) {
        return HAL_ERROR;
    }

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(htim, &master) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
}

HAL_StatusTypeDef Encoder_Init(void)
{
    if (Encoder_InitTimer(&s_htim1, TIM1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (Encoder_InitTimer(&s_htim2, TIM2) != HAL_OK) {
        return HAL_ERROR;
    }
    __HAL_TIM_SET_COUNTER(&s_htim1, 0U);
    __HAL_TIM_SET_COUNTER(&s_htim2, 0U);
    return HAL_OK;
}

int16_t Encoder_GetLeftDelta(void)
{
    int16_t delta = (int16_t)__HAL_TIM_GET_COUNTER(&s_htim1);
    __HAL_TIM_SET_COUNTER(&s_htim1, 0U);
    return -delta;
}

int16_t Encoder_GetRightDelta(void)
{
    int16_t delta = (int16_t)__HAL_TIM_GET_COUNTER(&s_htim2);
    __HAL_TIM_SET_COUNTER(&s_htim2, 0U);
    return delta;
}

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim_encoder)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    if (htim_encoder->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    } else if (htim_encoder->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    } else {
        return;
    }

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}
