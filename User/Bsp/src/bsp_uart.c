#include "bsp_uart.h"

/* DMA 接收缓冲区 */
static uint8_t dma_rx_buf[DMA_SIZE];

/* 回调函数指针 */
static Bsp_Uart_TxCpltCallback tx_cplt_cb = NULL;
static Bsp_Uart_RxCpltCallback rx_cplt_cb = NULL;

/**
 * @brief 注册发送完成回调
 * @param cb 回调函数指针
 */
void Bsp_Uart_RegisterTxCpltCallback(Bsp_Uart_TxCpltCallback cb)
{
    tx_cplt_cb = cb;
}

/**
 * @brief 注册接收完成回调
 * @param cb 回调函数指针
 */
void Bsp_Uart_RegisterRxCpltCallback(Bsp_Uart_RxCpltCallback cb)
{
    rx_cplt_cb = cb;
}

/**
 * @brief 启动 DMA 接收
 * @retval 0 成功, -1 失败
 */
int Bsp_Uart_StartRxDma(void)
{
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, DMA_SIZE) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief 启动 DMA 发送
 * @param data 发送数据指针
 * @param len 发送数据长度
 * @retval 0 成功, -1 失败
 */
int Bsp_Uart_StartTxDma(uint8_t *data, uint16_t len)
{
    if (HAL_UART_Transmit_DMA(&huart1, data, len) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief 中止 DMA 发送
 */
void Bsp_Uart_AbortTx(void)
{
    HAL_UART_AbortTransmit(&huart1);
}

/**
 * @brief 阻塞发送（仅用于初始化阶段调试，不使用DMA）
 * @param data 发送数据指针
 * @param len  发送数据长度
 */
void Bsp_Uart_Transmit(uint8_t *data, uint16_t len)
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
        if (tx_cplt_cb != NULL) {
            tx_cplt_cb();
        }
    }
}

/**
 * @brief UART 接收完成回调（DMA 不定长）
 * @note 当DMA接收到数据后，该回调函数会被调用。它会将接收到的数据通过回调通知上层，并重新启动DMA接收。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1 && Size > 0) {
        if (rx_cplt_cb != NULL) {
            rx_cplt_cb(dma_rx_buf, Size);
        }

        /* 重新启动 DMA 接收 */
        HAL_UARTEx_ReceiveToIdle_DMA(huart, dma_rx_buf, DMA_SIZE);
    }
}