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

/* 回调函数类型 */
typedef void (*Bsp_Can_TxCpltCallback)(void);
typedef void (*Bsp_Can_RxCpltCallback)(CAN_RxMsg_t *msg);

/* 回调注册函数 */
void Bsp_Can_RegisterTxCpltCallback(Bsp_Can_TxCpltCallback cb);
void Bsp_Can_RegisterRxCpltCallback(Bsp_Can_RxCpltCallback cb);

/* 纯硬件操作函数 */
int  Bsp_Can_Start(void);
int  Bsp_Can_StartTx(uint32_t id, uint8_t *data, uint8_t len);
void Bsp_Can_AbortTx(uint32_t tx_mailbox);
void Bsp_Can_ConfigFilter(uint32_t id, uint32_t mask);

#endif