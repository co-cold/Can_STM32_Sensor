#include "app_main.h"
#include "svc_interface.h"
#include "app_encoder.h"
#include "app_can.h"
#include "app_uart.h"

/* 任务属性（和 CubeMX 里配置的一致） */
const osThreadAttr_t encoderTask_attributes = {
  .name = "EncoderTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t canTask_attributes = {
  .name = "CanTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t uartTask_attributes = {
  .name = "UartTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

void App_Main(void)
{
    osThreadId_t hEnc, hCan, hUart;

    /* 初始化所有 Service 接口（内部依次调用 BSP 初始化） */
    Svc_Interface_Init();

    Svc_Uart_Printf(&svc_interface.uart, "[APP_MAIN] 所有接口初始化完成\r\n");

    /* 创建 FreeRTOS 任务 */
    hEnc = osThreadNew(App_Encoder_Task, NULL, &encoderTask_attributes);
    hCan = osThreadNew(App_Can_Rx_Task, NULL, &canTask_attributes);
    hUart = osThreadNew(App_Uart_Rx_Task, NULL, &uartTask_attributes);

    Svc_Uart_Printf(&svc_interface.uart, "[APP_MAIN] 任务句柄: enc=%p can=%p uart=%p\r\n", hEnc, hCan, hUart);
    Svc_Uart_Printf(&svc_interface.uart, "[APP_MAIN] 所有任务创建完成，应用启动成功\r\n");
}