#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

#include "can.h"
#include <stdint.h>

/* CAN 接收消息结构 */
typedef struct {
    uint32_t id;      // 标准帧ID
    uint8_t  data[8]; // 数据
    uint8_t  len;     // 数据长度 (0-8)
} CAN_RxMsg_t;

void Bsp_Can_Init(void);
void Bsp_Can_ConfigFilter(uint32_t id, uint32_t mask);
int Bsp_Can_Send(uint32_t id, uint8_t *data, uint8_t len, uint32_t timeout_ms);
int Bsp_Can_RecvFrame(CAN_RxMsg_t *msg, uint32_t timeout_ms);

#endif