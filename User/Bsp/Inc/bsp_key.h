#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include <stdint.h>

/* 按键事件（一次扫描最多返回一个事件，按优先级取最高） */
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_START,   /* 开始/暂停 按键 */
    KEY_EVENT_RESET,   /* 复位 按键 */
    KEY_EVENT_SW_SHORT,/* 编码器按键 短按（确认） */
    KEY_EVENT_SW_LONG, /* 编码器按键 长按（返回） */
} key_event_t;

void Bsp_Key_Init(void);

/**
 * @brief 按键轮询扫描（需周期调用，建议 10ms）
 * @note 内部做软件去抖，支持短按/长按判定
 */
key_event_t Bsp_Key_Scan(void);

#endif /* __BSP_KEY_H__ */