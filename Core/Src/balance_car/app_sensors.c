#include "balance_car/app_sensors.h"

#include "balance_car/dht11.h"
#include "balance_car/fsr_adc.h"

#define FSR_SAMPLE_PERIOD_MS    100U
#define DHT_SAMPLE_PERIOD_MS    2000U
#define DHT_START_LOW_MS        18U
#define DEFAULT_G_PER_COUNT     1.0f

volatile AppSensorState_t g_sensor_state = {
    .temperature_c = 0.0f,
    .humidity_percent = 0.0f,
    .fsr_adc_raw = 0U,
    .fsr_voltage_mv = 0U,
    .weight_g = 0.0f,
    .fsr_zero_adc = 0U,
    .fsr_g_per_count = DEFAULT_G_PER_COUNT,
    .sensor_fault_flags = SENSOR_FAULT_NONE,
};

static uint32_t s_next_fsr_ms;
static uint32_t s_next_dht_ms;
static uint32_t s_dht_start_ms;
static uint8_t s_dht_waiting;
static uint8_t s_fsr_zero_captured;

void AppSensors_SetFault(uint32_t fault)
{
    g_sensor_state.sensor_fault_flags |= fault;
}

void AppSensors_ClearFault(uint32_t fault)
{
    g_sensor_state.sensor_fault_flags &= ~fault;
}

HAL_StatusTypeDef AppSensors_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (Dht11_Init() != HAL_OK) {
        AppSensors_SetFault(SENSOR_FAULT_DHT_INIT);
        status = HAL_ERROR;
    }

    if (FsrAdc_Init() != HAL_OK) {
        AppSensors_SetFault(SENSOR_FAULT_FSR_INIT);
        status = HAL_ERROR;
    }

    s_next_fsr_ms = HAL_GetTick();
    s_next_dht_ms = HAL_GetTick() + 1000U;
    return status;
}

static void AppSensors_ReadFsr(void)
{
    uint16_t raw;
    uint16_t mv;
    float weight;

    if (FsrAdc_Read(&raw, &mv) != HAL_OK) {
        AppSensors_SetFault(SENSOR_FAULT_FSR_READ);
        g_sensor_state.fsr_valid = 0U;
        return;
    }

    AppSensors_ClearFault(SENSOR_FAULT_FSR_READ);
    g_sensor_state.fsr_valid = 1U;
    g_sensor_state.fsr_adc_raw = raw;
    g_sensor_state.fsr_voltage_mv = mv;

    if (s_fsr_zero_captured == 0U) {
        g_sensor_state.fsr_zero_adc = raw;
        s_fsr_zero_captured = 1U;
    }

    if (raw > g_sensor_state.fsr_zero_adc) {
        weight = (float)(raw - g_sensor_state.fsr_zero_adc) * g_sensor_state.fsr_g_per_count;
    } else {
        weight = 0.0f;
    }
    g_sensor_state.weight_g = weight;
}

static void AppSensors_ServiceDht(void)
{
    Dht11Reading_t reading;

    if (s_dht_waiting == 0U) {
        if (Dht11_StartRead() == HAL_OK) {
            s_dht_start_ms = HAL_GetTick();
            s_dht_waiting = 1U;
        } else {
            AppSensors_SetFault(SENSOR_FAULT_DHT_READ);
            s_next_dht_ms = HAL_GetTick() + DHT_SAMPLE_PERIOD_MS;
        }
        return;
    }

    if ((HAL_GetTick() - s_dht_start_ms) < DHT_START_LOW_MS) {
        return;
    }

    s_dht_waiting = 0U;
    s_next_dht_ms = HAL_GetTick() + DHT_SAMPLE_PERIOD_MS;

    if (Dht11_FinishRead(&reading) != HAL_OK) {
        AppSensors_SetFault(SENSOR_FAULT_DHT_READ);
        g_sensor_state.dht_valid = 0U;
        return;
    }

    AppSensors_ClearFault(SENSOR_FAULT_DHT_READ);
    g_sensor_state.dht_valid = 1U;
    g_sensor_state.temperature_c = reading.temperature_c;
    g_sensor_state.humidity_percent = reading.humidity_percent;
}

void AppSensors_Background(void)
{
    uint32_t now = HAL_GetTick();

    if ((int32_t)(now - s_next_fsr_ms) >= 0) {
        s_next_fsr_ms = now + FSR_SAMPLE_PERIOD_MS;
        AppSensors_ReadFsr();
    }

    if (s_dht_waiting != 0U || (int32_t)(now - s_next_dht_ms) >= 0) {
        AppSensors_ServiceDht();
    }
}
