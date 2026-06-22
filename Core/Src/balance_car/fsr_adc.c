#include "balance_car/fsr_adc.h"

#define FSR_ADC_MAX_COUNTS 4095U
#define FSR_ADC_VREF_MV    3300U

static ADC_HandleTypeDef s_hadc1;

HAL_StatusTypeDef FsrAdc_Init(void)
{
    ADC_ChannelConfTypeDef channel = {0};

    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV8);
    __HAL_RCC_ADC1_CLK_ENABLE();

    s_hadc1.Instance = ADC1;
    s_hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    s_hadc1.Init.ContinuousConvMode = DISABLE;
    s_hadc1.Init.DiscontinuousConvMode = DISABLE;
    s_hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    s_hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    s_hadc1.Init.NbrOfConversion = 1U;

    if (HAL_ADC_Init(&s_hadc1) != HAL_OK) {
        return HAL_ERROR;
    }

    channel.Channel = ADC_CHANNEL_2;
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&s_hadc1, &channel) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_ADCEx_Calibration_Start(&s_hadc1) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef FsrAdc_Read(uint16_t *adc_raw, uint16_t *voltage_mv)
{
    uint32_t value;

    if (adc_raw == 0 || voltage_mv == 0) {
        return HAL_ERROR;
    }

    if (HAL_ADC_Start(&s_hadc1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_ADC_PollForConversion(&s_hadc1, 5U) != HAL_OK) {
        (void)HAL_ADC_Stop(&s_hadc1);
        return HAL_TIMEOUT;
    }

    value = HAL_ADC_GetValue(&s_hadc1);
    (void)HAL_ADC_Stop(&s_hadc1);

    *adc_raw = (uint16_t)value;
    *voltage_mv = (uint16_t)((value * FSR_ADC_VREF_MV) / FSR_ADC_MAX_COUNTS);
    return HAL_OK;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef gpio = {0};

    if (hadc->Instance != ADC1) {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);
}
