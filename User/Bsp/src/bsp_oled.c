#include "bsp_oled.h"
#include "bsp_i2c.h"

/**
 * @brief 发送命令到 OLED
 * @note SSD1306 命令模式：控制字节 0x00 + 命令字节
 */
void Bsp_Oled_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    Bsp_I2c_MasterTransmit(OLED_I2C_ADDR, buf, 2);
}

/**
 * @brief 发送数据到 OLED
 * @note SSD1306 数据模式：控制字节 0x40 + 数据字节
 */
void Bsp_Oled_WriteData(uint8_t data)
{
    uint8_t buf[2] = {0x40, data};
    Bsp_I2c_MasterTransmit(OLED_I2C_ADDR, buf, 2);
}

/**
 * @brief 批量写显存数据
 * @note SSD1306 数据模式：控制字节 0x40 + 数据，用于全屏刷新
 */
void Bsp_Oled_WriteBuff(uint8_t *buf, uint16_t len)
{
    Bsp_I2c_MemWrite(OLED_I2C_ADDR, 0x40, buf, len);
}

/**
 * @brief OLED 初始化序列
 * @note SSD1306 标准初始化命令序列，参考数据手册
 */
void Bsp_Oled_InitSequence(void)
{
    Bsp_Oled_WriteCmd(0xAE); // 关闭显示

    Bsp_Oled_WriteCmd(0xD5); // 设置时钟分频/振荡器频率
    Bsp_Oled_WriteCmd(0x80);

    Bsp_Oled_WriteCmd(0xA8); // 设置多路复用率
    Bsp_Oled_WriteCmd(0x3F); // 1/64 duty

    Bsp_Oled_WriteCmd(0xD3); // 设置显示偏移
    Bsp_Oled_WriteCmd(0x00);

    Bsp_Oled_WriteCmd(0x40); // 设置显示起始行

    Bsp_Oled_WriteCmd(0x8D); // 设置电荷泵
    Bsp_Oled_WriteCmd(0x14); // 启用电荷泵

    Bsp_Oled_WriteCmd(0x20); // 设置内存寻址模式
    Bsp_Oled_WriteCmd(0x00); // 水平寻址模式

    Bsp_Oled_WriteCmd(0xA1); // 设置段重映射（左右翻转）
    Bsp_Oled_WriteCmd(0xC8); // 设置 COM 扫描方向（上下翻转）

    Bsp_Oled_WriteCmd(0xDA); // 设置 COM 引脚硬件配置
    Bsp_Oled_WriteCmd(0x12);

    Bsp_Oled_WriteCmd(0x81); // 设置对比度
    Bsp_Oled_WriteCmd(0xCF);

    Bsp_Oled_WriteCmd(0xD9); // 设置预充电周期
    Bsp_Oled_WriteCmd(0xF1);

    Bsp_Oled_WriteCmd(0xDB); // 设置 VCOMH 取消选择级别
    Bsp_Oled_WriteCmd(0x40);

    Bsp_Oled_WriteCmd(0xA4); // 全局显示开启（跟随 RAM）
    Bsp_Oled_WriteCmd(0xA6); // 设置正常显示模式（非反转）

    Bsp_Oled_WriteCmd(0x2E); // 停止滚动

    Bsp_Oled_WriteCmd(0xAF); // 开启显示

    HAL_Delay(100); // 初始化后等待稳定，此时调度器尚未启动
}