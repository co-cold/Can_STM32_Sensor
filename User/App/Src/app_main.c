#include "app_main.h"
#include "svc_interface.h"
#include "app_hmi.h"
#include "app_obd2.h"
#include "app_uart.h"

/* 任务属性（和 CubeMX 里配置的一致） */
const osThreadAttr_t hmiTask_attributes = {
  .name = "HmiTask",
  .stack_size = 1024,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t obd2Task_attributes = {
  .name = "Obd2Task",
  .stack_size = 1024,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t uartTask_attributes = {
  .name = "UartTask",
  .stack_size = 1024,
  .priority = (osPriority_t) osPriorityNormal,
};

void App_Main(void)
{
    osThreadId_t hHmi, hObd2, hUart;

    /* 初始化所有 Service 接口（内部依次调用 BSP 初始化） */
    Svc_Interface_Init();

    Svc_Uart_Printf(&svc_interface.uart, "[APP_MAIN] 所有接口初始化完成\r\n");

    /* 创建 FreeRTOS 任务 */
    hHmi = osThreadNew(App_Hmi_Task, NULL, &hmiTask_attributes);
    hObd2 = osThreadNew(App_Obd2_Task, NULL, &obd2Task_attributes);
    hUart = osThreadNew(App_Uart_Rx_Task, NULL, &uartTask_attributes);

    Svc_Uart_Printf(&svc_interface.uart, "[APP_MAIN] 任务句柄: hmi=%p obd2=%p uart=%p\r\n", hHmi, hObd2, hUart);
    Svc_Uart_Printf(&svc_interface.uart, "[APP_MAIN] 所有任务创建完成，应用启动成功\r\n");
}