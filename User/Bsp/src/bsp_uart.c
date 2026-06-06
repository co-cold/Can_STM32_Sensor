#include "bsp_uart.h"
#include "svc_interface.h"

#define UART_IF (svc_interface.uart)

/**
 * @brief UART 初始化
 * @note 该函数负责初始化UART接口，包括创建RTOS同步对象、初始化环形缓冲区，并启动DMA接收。
 */
void Bsp_Uart_Init(void)
{
    /* 创建发送互斥锁 */
    if(UART_IF.tx_mutex == NULL) {
            const osMutexAttr_t tx_mutex_attr = { .name = "UartTxMute"};
            UART_IF.tx_mutex = osMutexNew(&tx_mutex_attr);
            if (UART_IF.tx_mutex == NULL) {
                UART_IF.ConnectionError = 1; //标记初始化错误
                return;
                
            }
    }

    /* 创建发送信号量 */   
    if (UART_IF.tx_sem == NULL) {
        const osSemaphoreAttr_t tx_sem_attr = { .name = "UartTxSem" };
        UART_IF.tx_sem = osSemaphoreNew(1, 0, &tx_sem_attr);
        if (UART_IF.tx_sem == NULL) {
            UART_IF.ConnectionError = 1;
            return;
        }
    }

    /* 创建接收消息队列 */
    if (UART_IF.rx_queue == NULL) {
        UART_IF.rx_queue = osMessageQueueNew(8, sizeof(uint16_t), NULL);
        if (UART_IF.rx_queue == NULL) {
            UART_IF.ConnectionError = 1;
            return;
        }
    }

    /* 初始化环形缓冲区 */
    RingBuf_Init(&UART_IF.rx_ringbuf, UART_IF.rx_ringbuf_men, RINGBUF_SIZE);

    /* 启动 DMA 不定长接收 */
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UART_IF.dma_rx_buf, DMA_SIZE) != HAL_OK) {
        UART_IF.ConnectionError = 1;
        return;
    }
}

/**
 * @brief UART 发送数据（DMA）
 * @param data 待发送数据指针
 */
int Bsp_Uart_SendDma(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    if(data == NULL || len == 0) {
        return -1;
    }

    if(UART_IF.ConnectionError) {
        return -1;
    }

    /* 获取互斥锁，防止多任务 */
    if(osMutexAcquire(UART_IF.tx_mutex, timeout_ms) != osOK) {
        return -1;  //获取锁超时
    }

    osSemaphoreAcquire(UART_IF.tx_sem, 0);

    /* 启动DMA发送 */
    if(HAL_UART_Transmit_DMA(&huart1, data, len) != HAL_OK) {
        osMutexRelease(UART_IF.tx_mutex);
        return -1;
    }

    /* 等待发送完成信号 */
    if(osSemaphoreAcquire(UART_IF.tx_sem, timeout_ms) != osOK) {
        HAL_UART_AbortTransmit(&huart1);  //超时就中止DMA
        osMutexRelease(UART_IF.tx_mutex);
        return -1;  //发送完成信号超时
    }
   
    /* 发送完成，释放互斥锁 */
    osMutexRelease(UART_IF.tx_mutex);
    return (int)len;
}

/**
 * @brief UART 接收数据帧
 * @param data 接收数据缓冲区指针
 */
int Bsp_Uart_RecvFrame(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    uint16_t frame_len = 0;

    if(data == NULL || len == 0) {
        return -1;
    }

    if(UART_IF.ConnectionError) {
        return -1;
    }

    /* 等待消息队列 */
    if(osMessageQueueGet(UART_IF.rx_queue, &frame_len, NULL, timeout_ms) != osOK) {
        return -1;  //超时
    }

    /* 防止越界 */
    if(frame_len > len) {
        frame_len = len;
    }

    return (int)RingBuf_Read(&UART_IF.rx_ringbuf, data, frame_len);
}

/**
 * @brief UART 可用数据长度
 */
uint16_t Bsp_Uart_RxAvailable(void)
{
    return RingBuf_Available(&UART_IF.rx_ringbuf);
}

/**
 * @brief UART 阻塞发送（只用于初始化阶段调试，不使用DMA）
 */
void Bsp_Uart_SendBlocking(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart1, data, len, HAL_MAX_DELAY);
}

/******* HAL 库回调函数 ********/

/**
 * @brief UART 发送完成回调（DMA）
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (UART_IF.tx_sem != NULL) {
            osSemaphoreRelease(UART_IF.tx_sem);
        }
    }
}

/**
 * @brief UART 接收完成回调（DMA）
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1 && Size > 0) {
        /* 将接收到的数据写入环形缓冲区 */
        RingBuf_Write(&UART_IF.rx_ringbuf, UART_IF.dma_rx_buf, Size);

        /* 发送消息通知有新数据 */
        if (UART_IF.rx_queue != NULL) {
            osMessageQueuePut(UART_IF.rx_queue, &Size, 0, 0);
        }

        /* 重新启动 DMA 接收 */
        HAL_UARTEx_ReceiveToIdle_DMA(huart, UART_IF.dma_rx_buf, DMA_SIZE);
    }
}