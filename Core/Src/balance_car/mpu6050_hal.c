#include "balance_car/mpu6050_hal.h"

#include "balance_car/i2c_bus.h"

#define MPU6050_ADDR              (0x68U << 1)
#define MPU6050_WHO_AM_I          0x75U
#define MPU6050_PWR_MGMT_1        0x6BU
#define MPU6050_PWR_MGMT_2        0x6CU
#define MPU6050_SMPLRT_DIV        0x19U
#define MPU6050_CONFIG            0x1AU
#define MPU6050_GYRO_CONFIG       0x1BU
#define MPU6050_ACCEL_CONFIG      0x1CU
#define MPU6050_ACCEL_XOUT_H      0x3BU

static uint8_t s_last_id;

static HAL_StatusTypeDef Mpu6050_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(BalanceI2C1_GetHandle(), MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                             &value, 1U, 10U);
}

static HAL_StatusTypeDef Mpu6050_ReadReg(uint8_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(BalanceI2C1_GetHandle(), MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                            value, 1U, 10U);
}

HAL_StatusTypeDef Mpu6050_Init(void)
{
    if (BalanceI2C1_Init() != HAL_OK) {
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

    if (HAL_I2C_Mem_Read(BalanceI2C1_GetHandle(), MPU6050_ADDR, MPU6050_ACCEL_XOUT_H,
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
