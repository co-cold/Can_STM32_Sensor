#ifndef __SVC_UART_H__
#define __SVC_UART_H__

#include "bsp_uart.h"
#include "ringbuf.h"
#include <stdarg.h>
#include "cmsis_os.h"

#define RINGBUF_SIZE 512

/**
 * @brief UART服务接口
 * @note 该接口封装了UART的发送和接收功能，提供给上层应用使用。
 */
typedef struct
{
    uint8_t ConnectionError;    //初始化错误标记

    /* 数据缓冲区 */
    RingBuf_t rx_ringbuf;
    uint8_t rx_ringbuf_men[RINGBUF_SIZE];

    /* RTOS 同步 */
    osMutexId_t tx_mutex;
    osSemaphoreId_t tx_sem;
    osMessageQueueId_t rx_queue;

    /* 接口函数 */
    int (*Send)(uint8_t *data, uint16_t len, uint32_t timeout_ms);
    int (*RecvFrame)(uint8_t *buf, uint16_t buf_size, uint32_t timeout_ms);
    uint16_t (*Available)(void);
    void (*SendBlocking)(uint8_t *data, uint16_t len);
} svc_uart_t;

void Svc_Uart_InitInstance(svc_uart_t *pIf);

void Svc_Uart_Printf(svc_uart_t *uart, const char *format, ...);

#endif