#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "usart.h"

#define DMA_SIZE 256

/* 回调函数类型 */
typedef void (*Bsp_Uart_TxCpltCallback)(void);
typedef void (*Bsp_Uart_RxCpltCallback)(uint8_t *data, uint16_t len);

/* 回调注册函数 */
void Bsp_Uart_RegisterTxCpltCallback(Bsp_Uart_TxCpltCallback cb);
void Bsp_Uart_RegisterRxCpltCallback(Bsp_Uart_RxCpltCallback cb);

/* 纯硬件操作函数 */
int  Bsp_Uart_StartRxDma(void);
int  Bsp_Uart_StartTxDma(uint8_t *data, uint16_t len);
void Bsp_Uart_AbortTx(void);
void Bsp_Uart_Transmit(uint8_t *data, uint16_t len);

#endif