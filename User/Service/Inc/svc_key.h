#ifndef __SVC_KEY_H__
#define __SVC_KEY_H__

#include <stdint.h>
#include <stddef.h>
#include "bsp_key.h"

/**
 * @brief 按键服务接口
 * @note 封装 BSP 按键扫描，供上层应用轮询事件
 */
typedef struct {
    uint8_t ConnectionError;             /* 初始化错误标记 */
    key_event_t (*Scan)(void);           /* 按键事件扫描 */
} svc_key_t;

void Svc_Key_InitInstance(svc_key_t *pIf);

#endif /* __SVC_KEY_H__ */