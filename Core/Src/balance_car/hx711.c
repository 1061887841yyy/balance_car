#include "balance_car/hx711.h"

#define HX711_DT_PORT          GPIOB
#define HX711_DT_PIN           GPIO_PIN_12
#define HX711_SCK_PORT         GPIOB
#define HX711_SCK_PIN          GPIO_PIN_13

static uint8_t s_dwt_ready;

static void Hx711_DwtInit(void)
{
    if (s_dwt_ready != 0U) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    s_dwt_ready = 1U;
}

static void Hx711_DelayUs(uint32_t us)
{
    uint32_t cycles;
    uint32_t start;

    if (us == 0U) {
        return;
    }

    cycles = (HAL_RCC_GetHCLKFreq() / 1000000U) * us;
    start = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
    }
}

HAL_StatusTypeDef Hx711_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    Hx711_DwtInit();

    gpio.Pin = HX711_DT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HX711_DT_PORT, &gpio);

    gpio.Pin = HX711_SCK_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HX711_SCK_PORT, &gpio);

    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
    return HAL_OK;
}

HAL_StatusTypeDef Hx711_ReadRaw(int32_t *raw)
{
    uint32_t data = 0U;

    if (raw == 0) {
        return HAL_ERROR;
    }

    if (HAL_GPIO_ReadPin(HX711_DT_PORT, HX711_DT_PIN) == GPIO_PIN_SET) {
        return HAL_BUSY;
    }

    for (uint8_t bit = 0U; bit < 24U; bit++) {
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
        Hx711_DelayUs(1U);
        data = (data << 1) | ((HAL_GPIO_ReadPin(HX711_DT_PORT, HX711_DT_PIN) == GPIO_PIN_SET) ? 1U : 0U);
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
        Hx711_DelayUs(1U);
    }

    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
    Hx711_DelayUs(1U);
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
    Hx711_DelayUs(1U);

    if ((data & 0x800000UL) != 0U) {
        data |= 0xFF000000UL;
    }
    *raw = (int32_t)data;
    return HAL_OK;
}

void Hx711_PowerDown(void)
{
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
    Hx711_DelayUs(80U);
}

void Hx711_PowerUp(void)
{
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
}
