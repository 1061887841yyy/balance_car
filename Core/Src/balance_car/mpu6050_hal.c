#include "balance_car/mpu6050_hal.h"

#define MPU6050_ADDR              (0x68U << 1)
#define MPU6050_WHO_AM_I          0x75U
#define MPU6050_PWR_MGMT_1        0x6BU
#define MPU6050_PWR_MGMT_2        0x6CU
#define MPU6050_SMPLRT_DIV        0x19U
#define MPU6050_CONFIG            0x1AU
#define MPU6050_GYRO_CONFIG       0x1BU
#define MPU6050_ACCEL_CONFIG      0x1CU
#define MPU6050_ACCEL_XOUT_H      0x3BU

static I2C_HandleTypeDef s_hi2c1;
static uint8_t s_last_id;

static HAL_StatusTypeDef Mpu6050_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&s_hi2c1, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                             &value, 1U, 10U);
}

static HAL_StatusTypeDef Mpu6050_ReadReg(uint8_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(&s_hi2c1, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                            value, 1U, 10U);
}

HAL_StatusTypeDef Mpu6050_Init(void)
{
    s_hi2c1.Instance = I2C1;
    s_hi2c1.Init.ClockSpeed = 400000U;
    s_hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    s_hi2c1.Init.OwnAddress1 = 0U;
    s_hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_hi2c1.Init.OwnAddress2 = 0U;
    s_hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_hi2c1) != HAL_OK) {
        return HAL_ERROR;
    }

    if (Mpu6050_ReadReg(MPU6050_WHO_AM_I, &s_last_id) != HAL_OK) {
        return HAL_ERROR;
    }
    if (s_last_id != 0x68U) {
        return HAL_ERROR;
    }

    if (Mpu6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01U) != HAL_OK) {
        return HAL_ERROR;
    }
    if (Mpu6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00U) != HAL_OK) {
        return HAL_ERROR;
    }
    if (Mpu6050_WriteReg(MPU6050_SMPLRT_DIV, 0x07U) != HAL_OK) {
        return HAL_ERROR;
    }
    if (Mpu6050_WriteReg(MPU6050_CONFIG, 0x00U) != HAL_OK) {
        return HAL_ERROR;
    }
    if (Mpu6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18U) != HAL_OK) {
        return HAL_ERROR;
    }
    if (Mpu6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18U) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Mpu6050_ReadRaw(Mpu6050Raw_t *raw)
{
    uint8_t data[14];

    if (HAL_I2C_Mem_Read(&s_hi2c1, MPU6050_ADDR, MPU6050_ACCEL_XOUT_H,
                         I2C_MEMADD_SIZE_8BIT, data, sizeof(data), 5U) != HAL_OK) {
        return HAL_ERROR;
    }

    raw->ax = (int16_t)((uint16_t)data[0] << 8 | data[1]);
    raw->ay = (int16_t)((uint16_t)data[2] << 8 | data[3]);
    raw->az = (int16_t)((uint16_t)data[4] << 8 | data[5]);
    raw->gx = (int16_t)((uint16_t)data[8] << 8 | data[9]);
    raw->gy = (int16_t)((uint16_t)data[10] << 8 | data[11]);
    raw->gz = (int16_t)((uint16_t)data[12] << 8 | data[13]);

    return HAL_OK;
}

uint8_t Mpu6050_GetLastId(void)
{
    return s_last_id;
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2c->Instance != I2C1) {
        return;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_AFIO_REMAP_I2C1_ENABLE();

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
}
