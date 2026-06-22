#ifndef BALANCE_CAR_APP_SENSORS_H
#define BALANCE_CAR_APP_SENSORS_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef enum {
    SENSOR_FAULT_NONE = 0x00000000UL,
    SENSOR_FAULT_DHT_INIT = 0x00000001UL,
    SENSOR_FAULT_DHT_READ = 0x00000002UL,
    SENSOR_FAULT_FSR_INIT = 0x00000004UL,
    SENSOR_FAULT_FSR_READ = 0x00000008UL,
    SENSOR_FAULT_OLED_INIT = 0x00000010UL,
    SENSOR_FAULT_OLED_REFRESH = 0x00000020UL
} SensorFault_t;

typedef struct {
    float temperature_c;
    float humidity_percent;
    uint16_t fsr_adc_raw;
    uint16_t fsr_voltage_mv;
    float weight_g;
    uint16_t fsr_zero_adc;
    float fsr_g_per_count;
    uint32_t sensor_fault_flags;
    uint8_t dht_valid;
    uint8_t fsr_valid;
    uint8_t oled_ready;
    uint8_t oled_addr_7bit;
    uint8_t oled_probe_mask;
    uint8_t oled_fail_step;
    uint8_t reserved;
} AppSensorState_t;

extern volatile AppSensorState_t g_sensor_state;

HAL_StatusTypeDef AppSensors_Init(void);
void AppSensors_Background(void);
void AppSensors_SetFault(uint32_t fault);
void AppSensors_ClearFault(uint32_t fault);

#endif
