#ifndef __SVC_OLED_H__
#define __SVC_OLED_H__

#include "cmsis_os.h"
#include "bsp_oled.h"

#define OLED_FB_SIZE  (OLED_WIDTH * OLED_PAGES)  // 显存大小：1KB

/**
 * @brief OLED 设备服务
 * @note 管理显存和设备锁，提供画点、字符串显示、刷新等功能
 */
typedef struct {
    uint8_t framebuffer[OLED_FB_SIZE];  // 显存：静态分配，不占任务栈
    osMutexId_t fb_mutex;               // 设备级锁：保护显存多任务访问
} svc_oled_t;

/**
 * @brief OLED 设备初始化
 * @note 创建显存锁，清空显存，调用 BSP 初始化，首次刷新
 * @param oled OLED 设备实例指针
 */
void Svc_Oled_Init(svc_oled_t *oled);

/**
 * @brief 刷新显存到屏幕（全屏）
 * @note 持有总线锁和设备锁，I2C 400kbps 下约耗时 20ms
 * @param oled OLED 设备实例指针
 */
void Svc_Oled_Refresh(svc_oled_t *oled);

/**
 * @brief 画点
 * @note 仅修改显存，不刷新屏幕，需调用 Refresh 才能显示
 * @param oled OLED 设备实例指针
 * @param x 横坐标（0~127）
 * @param y 纵坐标（0~63）
 * @param color 1=亮, 0=灭
 */
void Svc_Oled_DrawPixel(svc_oled_t *oled, uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief 显示字符串（5x7 ASCII 字体）
 * @note 仅修改显存，不刷新屏幕
 * @param oled OLED 设备实例指针
 * @param x 起始横坐标（0~127，自动换行）
 * @param page 起始页（0~7，每页 8 像素高）
 * @param str 字符串指针
 */
void Svc_Oled_ShowString(svc_oled_t *oled, uint8_t x, uint8_t page, const char *str);

/**
 * @brief 清空显存
 * @note 仅修改显存，不刷新屏幕
 * @param oled OLED 设备实例指针
 */
void Svc_Oled_Clear(svc_oled_t *oled);

#endif