#include "balance_car/dht11.h"

#define DHT11_PORT GPIOC
#define DHT11_PIN  GPIO_PIN_14

static uint8_t s_dwt_ready;
static uint8_t s_fail_step;

static void Dht11_SetOutput(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = DHT11_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &gpio);
}

static void Dht11_SetInput(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = DHT11_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &gpio);
}

static void Dht11_DwtInit(void)
{
    if (s_dwt_ready != 0U) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    s_dwt_ready = 1U;
}

static uint32_t Dht11_Micros(void)
{
    return DWT->CYCCNT / (HAL_RCC_GetHCLKFreq() / 1000000U);
}

static HAL_StatusTypeDef Dht11_WaitForState(GPIO_PinState state, uint32_t timeout_us)
{
    uint32_t start = Dht11_Micros();

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != state) {
        if ((Dht11_Micros() - start) > timeout_us) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef Dht11_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    Dht11_DwtInit();
    Dht11_SetInput();
    return HAL_OK;
}

HAL_StatusTypeDef Dht11_StartRead(void)
{
    s_fail_step = 0U;
    Dht11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    return HAL_OK;
}

HAL_StatusTypeDef Dht11_FinishRead(Dht11Reading_t *reading)
{
    uint8_t data[5] = {0};

    if (reading == 0) {
        return HAL_ERROR;
    }

    Dht11_SetInput();
    if (Dht11_WaitForState(GPIO_PIN_RESET, 100U) != HAL_OK) {
        s_fail_step = 1U;
        return HAL_TIMEOUT;
    }
    if (Dht11_WaitForState(GPIO_PIN_SET, 100U) != HAL_OK) {
        s_fail_step = 2U;
        return HAL_TIMEOUT;
    }
    if (Dht11_WaitForState(GPIO_PIN_RESET, 100U) != HAL_OK) {
        s_fail_step = 3U;
        return HAL_TIMEOUT;
    }

    for (uint8_t bit = 0U; bit < 40U; bit++) {
        uint32_t high_start;
        uint32_t high_width;

        if (Dht11_WaitForState(GPIO_PIN_SET, 80U) != HAL_OK) {
            s_fail_step = 4U;
            return HAL_TIMEOUT;
        }
        high_start = Dht11_Micros();
        if (Dht11_WaitForState(GPIO_PIN_RESET, 120U) != HAL_OK) {
            s_fail_step = 5U;
            return HAL_TIMEOUT;
        }
        high_width = Dht11_Micros() - high_start;
        data[bit / 8U] <<= 1;
        if (high_width > 45U) {
            data[bit / 8U] |= 1U;
        }
    }

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) {
        s_fail_step = 6U;
        return HAL_ERROR;
    }

    reading->humidity_percent = (float)data[0] + (float)data[1] * 0.1f;
    reading->temperature_c = (float)data[2] + (float)data[3] * 0.1f;
    s_fail_step = 0U;
    return HAL_OK;
}

uint8_t Dht11_GetFailStep(void)
{
    return s_fail_step;
}
