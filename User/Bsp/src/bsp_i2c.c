#include "bsp_i2c.h"

/**
 * @brief I2C 主设备发送
 * @note 纯 HAL 封装，无 RTOS 依赖，7位地址自动左移
 */
int Bsp_I2c_MasterTransmit(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(dev_addr << 1), data, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief I2C 主设备接收
 * @note 纯 HAL 封装，无 RTOS 依赖
 */
int Bsp_I2c_MasterReceive(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(dev_addr << 1), data, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief I2C 存储器写（带寄存器地址）
 * @note 纯 HAL 封装，无 RTOS 依赖。发送序列：start → dev_addr(w) → mem_addr → data... → stop
 */
int Bsp_I2c_MemWrite(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1), mem_addr,
                          I2C_MEMADD_SIZE_8BIT, data, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief I2C 存储器读（带寄存器地址）
 * @note 纯 HAL 封装，无 RTOS 依赖
 */
int Bsp_I2c_MemRead(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), mem_addr,
                         I2C_MEMADD_SIZE_8BIT, data, len, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}