#ifndef __BSP_I2C_H__
#define __BSP_I2C_H__

#include "i2c.h"

/**
 * @brief I2C 主设备发送
 * @param dev_addr 7位设备地址
 * @param data 发送数据指针
 * @param len 发送数据长度
 * @retval 0 成功, -1 失败
 */
int Bsp_I2c_MasterTransmit(uint8_t dev_addr, uint8_t *data, uint16_t len);

/**
 * @brief I2C 主设备接收
 * @param dev_addr 7位设备地址
 * @param data 接收数据缓冲区
 * @param len 接收数据长度
 * @retval 0 成功, -1 失败
 */
int Bsp_I2c_MasterReceive(uint8_t dev_addr, uint8_t *data, uint16_t len);

/**
 * @brief I2C 存储器写（带寄存器地址）
 * @param dev_addr 7位设备地址
 * @param mem_addr 寄存器/命令地址
 * @param data 发送数据指针
 * @param len 发送数据长度
 * @retval 0 成功, -1 失败
 */
int Bsp_I2c_MemWrite(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint16_t len);

/**
 * @brief I2C 存储器读（带寄存器地址）
 * @param dev_addr 7位设备地址
 * @param mem_addr 寄存器/命令地址
 * @param data 接收数据缓冲区
 * @param len 接收数据长度
 * @retval 0 成功, -1 失败
 */
int Bsp_I2c_MemRead(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, uint16_t len);

#endif