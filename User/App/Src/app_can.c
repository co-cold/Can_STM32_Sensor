#include "app_can.h"
#include "svc_interface.h"

void App_Can_Rx_Task(void *argument)
{
    CAN_RxMsg_t rx_msg;
    uint8_t i;

    Svc_Uart_Printf(&svc_interface.uart, "[CAN_RX] CAN接收任务已启动\r\n");

    while (1)
    {
        if (svc_interface.can.RecvFrame(&rx_msg, osWaitForever) > 0)
        {
            Svc_Uart_Printf(&svc_interface.uart,
                "[CAN_RX] ID=0x%03X, 长度=%d, 数据:", rx_msg.id, rx_msg.len);

            for (i = 0; i < rx_msg.len; i++)
            {
                Svc_Uart_Printf(&svc_interface.uart, " %02X", rx_msg.data[i]);
            }
            Svc_Uart_Printf(&svc_interface.uart, "\r\n");
        }
        osDelay(1000);
    }
	vTaskDelete(NULL);
}