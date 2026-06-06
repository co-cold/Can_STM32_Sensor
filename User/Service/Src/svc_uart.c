#include "svc_uart.h"
#include "bsp_uart.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief UART 服务接口实例
 * @note 该实例在 Svc_Interface_Init() 中进行初始化，提供给上层应用使用。
 */
void Svc_Uart_InitInstance(svc_uart_t *pIf)
{
    /* 挂载函数指针 */
    pIf->Init          = Bsp_Uart_Init;
    pIf->Send          = Bsp_Uart_SendDma;
    pIf->RecvFrame     = Bsp_Uart_RecvFrame;
    pIf->Available     = Bsp_Uart_RxAvailable;
    pIf->SendBlocking  = Bsp_Uart_SendBlocking;

    /* 调用 BSP 初始化 */
    pIf->Init();
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

    uart->SendBlocking((uint8_t *)buf, (uint16_t)len);
}