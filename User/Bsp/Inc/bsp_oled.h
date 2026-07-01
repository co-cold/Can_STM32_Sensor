#ifndef __BSP_OLED_H__
#define __BSP_OLED_H__

#include <stdint.h>

#define OLED_I2C_ADDR  0x3C       // SSD1306 7位I2C地址
#define OLED_WIDTH     128        // 显示宽度（像素）
#define OLED_HEIGHT    64         // 显示高度（像素）
#define OLED_PAGES     (OLED_HEIGHT / 8)  // 页数（8页）

/**
 * @brief 发送命令到 OLED
 * @param cmd 命令字节
 */
void Bsp_Oled_WriteCmd(uint8_t cmd);

/**
 * @brief 发送数据到 OLED
 * @param data 数据字节
 */
void Bsp_Oled_WriteData(uint8_t data);

/**
 * @brief 批量写显存数据
 * @param buf 数据缓冲区
 * @param len 数据长度
 */
void Bsp_Oled_WriteBuff(uint8_t *buf, uint16_t len);

/**
 * @brief OLED 初始化序列
 * @note 发送 SSD1306 标准初始化命令序列，需在调度器启动前调用
 */
void Bsp_Oled_InitSequence(void);

#endif