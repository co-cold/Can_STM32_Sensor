#include "app_uart.h"
#include "svc_interface.h"

#define APP_UART_RX_BUF_SIZE 256

void App_Uart_Rx_Task(void *argument)
{
    uint8_t buf[APP_UART_RX_BUF_SIZE];
    int len;

    Svc_Uart_Printf(&svc_interface.uart, "[APP_UART] 串口接收任务已启动\r\n");

    while (1)
    {
        len = svc_interface.uart.RecvFrame(buf, sizeof(buf), osWaitForever);
        if (len > 0)
        {
            buf[len] = '\0';
            Svc_Uart_Printf(&svc_interface.uart,
                "[APP_UART] 收到(%d字节): %s\r\n", len, buf);
        }
    }
}