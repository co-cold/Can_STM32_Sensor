#ifndef __SVC_CAN_H__
#define __SVC_CAN_H__

#include <stdint.h>
#include "cmsis_os.h"
#include "bsp_can.h"

/**
 * @brief CAN服务接口
 * @note 该接口封装了CAN的发送和接收功能，提供给上层应用使用。
 */
typedef struct 
{   
    uint8_t ConnectionError;    //初始化错误标记

    /* RTOS 同步 */
    osMutexId_t tx_mutex;
    osSemaphoreId_t tx_sem;
    osMessageQueueId_t rx_queue;

    /* 接口函数 */
    int  (*Send)(uint32_t id, uint8_t *data, uint8_t len, uint32_t timeout_ms);
    int  (*RecvFrame)(CAN_RxMsg_t *msg, uint32_t timeout_ms);
} svc_can_t;

void Svc_Can_InitInstance(svc_can_t *pIf);

#endif