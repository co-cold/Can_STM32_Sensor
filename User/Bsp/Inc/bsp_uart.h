#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "usart.h"  //Hal库

void Bsp_Uart_Init(void);
int Bsp_Uart_SendDma(uint8_t *data, uint16_t len, uint32_t timeout_ms);
int Bsp_Uart_RecvFrame(uint8_t *data, uint16_t len, uint32_t timeout_ms);
uint16_t Bsp_Uart_RxAvailable(void);
void Bsp_Uart_SendBlocking(uint8_t *data, uint16_t len);

#endif