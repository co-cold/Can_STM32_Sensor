#include "svc_uart.h"
#include "bsp_uart.h"
#include <stdio.h>
#include <string.h>

/* 当前操作的 UART 接口实例指针（用于回调中访问 RTOS 对象） */
static svc_uart_t *g_uart_if = NULL;

/**
 * @brief 发送完成回调（注册给 BSP 层）
 * @note 当 DMA 发送完成后，BSP 层调用此回调，释放 tx_sem 信号量
 */
static void Svc_Uart_TxComplete(void)
{
    if (g_uart_if != NULL && g_uart_if->tx_sem != NULL) {
        osSemaphoreRelease(g_uart_if->tx_sem);
    }
}

/**
 * @brief 接收完成回调（注册给 BSP 层）
 * @note 当 DMA 接收到数据后，BSP 层调用此回调，将数据写入环形缓冲区并发送消息队列通知
 */
static void Svc_Uart_RxComplete(uint8_t *data, uint16_t len)
{
    if (g_uart_if == NULL) return;

    /* 将接收到的数据写入环形缓冲区 */
    RingBuf_Write(&g_uart_if->rx_ringbuf, data, len);

    /* 发送消息通知有新数据 */
    if (g_uart_if->rx_queue != NULL) {
        osMessageQueuePut(g_uart_if->rx_queue, &len, 0, 0);
    }
}

/**
 * @brief UART 发送数据（DMA，带锁）
 * @param data 待发送数据指针
 * @param len 待发送数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @retval >0 发送成功, -1 失败
 */
static int Svc_Uart_Send(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    if (data == NULL || len == 0) return -1;
    if (g_uart_if == NULL || g_uart_if->ConnectionError) return -1;

    /* 获取互斥锁，防止多任务同时发送 */
    if (osMutexAcquire(g_uart_if->tx_mutex, timeout_ms) != osOK) {
        return -1;
    }

    osSemaphoreAcquire(g_uart_if->tx_sem, 0);

    /* 启动 DMA 发送 */
    if (Bsp_Uart_StartTxDma(data, len) != 0) {
        osMutexRelease(g_uart_if->tx_mutex);
        return -1;
    }

    /* 等待发送完成信号 */
    if (osSemaphoreAcquire(g_uart_if->tx_sem, timeout_ms) != osOK) {
        Bsp_Uart_AbortTx();
        osMutexRelease(g_uart_if->tx_mutex);
        return -1;
    }

    osMutexRelease(g_uart_if->tx_mutex);
    return (int)len;
}

/**
 * @brief UART 接收数据帧
 * @param buf 接收数据缓冲区指针
 * @param buf_size 缓冲区大小
 * @param timeout_ms 超时时间（毫秒）
 * @retval >0 实际接收长度, -1 失败
 */
static int Svc_Uart_RecvFrame(uint8_t *buf, uint16_t buf_size, uint32_t timeout_ms)
{
    uint16_t frame_len = 0;

    if (buf == NULL || buf_size == 0) return -1;
    if (g_uart_if == NULL || g_uart_if->ConnectionError) return -1;

    /* 等待消息队列 */
    if (osMessageQueueGet(g_uart_if->rx_queue, &frame_len, NULL, timeout_ms) != osOK) {
        return -1;
    }

    /* 防止越界 */
    if (frame_len > buf_size) {
        frame_len = buf_size;
    }

    return (int)RingBuf_Read(&g_uart_if->rx_ringbuf, buf, frame_len);
}

/**
 * @brief UART 可用数据长度
 */
static uint16_t Svc_Uart_Available(void)
{
    if (g_uart_if == NULL) return 0;
    return RingBuf_Available(&g_uart_if->rx_ringbuf);
}

/**
 * @brief UART 阻塞发送（不使用DMA，用于初始化阶段调试）
 */
static void Svc_Uart_SendBlocking(uint8_t *data, uint16_t len)
{
    Bsp_Uart_Transmit(data, len);
}

/**
 * @brief UART 服务接口实例初始化
 * @note 该实例在 Svc_Interface_Init() 中进行初始化，提供给上层应用使用。
 */
void Svc_Uart_InitInstance(svc_uart_t *pIf)
{
    g_uart_if = pIf;

    /* 创建发送互斥锁 */
    if (pIf->tx_mutex == NULL) {
        const osMutexAttr_t tx_mutex_attr = { .name = "UartTxMute" };
        pIf->tx_mutex = osMutexNew(&tx_mutex_attr);
        if (pIf->tx_mutex == NULL) {
            pIf->ConnectionError = 1;
            return;
        }
    }

    /* 创建发送完成信号量 */
    if (pIf->tx_sem == NULL) {
        const osSemaphoreAttr_t tx_sem_attr = { .name = "UartTxSem" };
        pIf->tx_sem = osSemaphoreNew(1, 0, &tx_sem_attr);
        if (pIf->tx_sem == NULL) {
            pIf->ConnectionError = 1;
            return;
        }
    }

    /* 创建接收消息队列 */
    if (pIf->rx_queue == NULL) {
        pIf->rx_queue = osMessageQueueNew(8, sizeof(uint16_t), NULL);
        if (pIf->rx_queue == NULL) {
            pIf->ConnectionError = 1;
            return;
        }
    }

    /* 初始化环形缓冲区 */
    RingBuf_Init(&pIf->rx_ringbuf, pIf->rx_ringbuf_men, RINGBUF_SIZE);

    /* 注册回调到 BSP 层 */
    Bsp_Uart_RegisterTxCpltCallback(Svc_Uart_TxComplete);
    Bsp_Uart_RegisterRxCpltCallback(Svc_Uart_RxComplete);

    /* 挂载函数指针 */
    pIf->Send          = Svc_Uart_Send;
    pIf->RecvFrame     = Svc_Uart_RecvFrame;
    pIf->Available     = Svc_Uart_Available;
    pIf->SendBlocking  = Svc_Uart_SendBlocking;
}

/**
 * @brief 启动 DMA 接收
 * @note 在 OLED 初始化完成后、调度器启动后调用，避免中断干扰 OLED SPI 时序
 */
void Svc_Uart_StartRx(void)
{
    if (g_uart_if == NULL || g_uart_if->ConnectionError) return;
    Bsp_Uart_StartRxDma();
}

#define PRINTF_BUF_SIZE 256

void Svc_Uart_Printf(svc_uart_t *uart, const char *format, ...)
{
    char buf[PRINTF_BUF_SIZE];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len <= 0) return;
    if (len >= (int)sizeof(buf)) {
        len = (int)sizeof(buf) - 1;
    }

    if (osKernelGetState() == osKernelRunning) {
        uart->Send((uint8_t *)buf, (uint16_t)len, osWaitForever);
    } else {
        uart->SendBlocking((uint8_t *)buf, (uint16_t)len);
    }
}