#include "app_encoder.h"
#include "svc_interface.h"
#include "svc_oled.h"
#include "cmsis_os.h"
#include <stdio.h>

/* OLED 设备实例：静态分配，不占任务栈（1KB 显存） */
static svc_oled_t oled_dev;
static uint8_t oled_inited = 0;

void App_Encoder_Task(void *argument)
{
    Encoder_Data_t data;
    uint8_t can_data[8];
    int16_t speed_i, angle_i;
    char buf[22];

    Svc_Uart_Printf(&svc_interface.uart, "[ENCODER] 编码器任务已启动\r\n");

    /* 初始化 OLED */
    if (!oled_inited) {
        Svc_Oled_Init(&oled_dev);
        oled_inited = 1;
        Svc_Uart_Printf(&svc_interface.uart, "[ENCODER] OLED 初始化完成\r\n");
    }

    while (1)
    {
        data = svc_interface.encoder.GetData();

        // 打包CAN数据
        speed_i = (int16_t)(data.speed_rpm * 10.0f);
        angle_i = (int16_t)(data.angle * 10.0f);

        can_data[0] = (uint8_t)(data.count & 0xFF);
        can_data[1] = (uint8_t)((data.count >> 8) & 0xFF);
        can_data[2] = (uint8_t)data.dir;
        can_data[3] = (uint8_t)(speed_i & 0xFF);
        can_data[4] = (uint8_t)((speed_i >> 8) & 0xFF);
        can_data[5] = (uint8_t)(angle_i & 0xFF);
        can_data[6] = (uint8_t)((angle_i >> 8) & 0xFF);
        can_data[7] = 0;

        Svc_Uart_Printf(&svc_interface.uart, "[ENCODER] 准备发送CAN\r\n");

        if (svc_interface.can.Send(0x100, can_data, 8, 100) > 0)
        {
            Svc_Uart_Printf(&svc_interface.uart,
                "[ENCODER] 发送成功: 计数=%d, 方向=%d, 速度=%.1f RPM, 角度=%.1f度\r\n",
                data.count, data.dir, data.speed_rpm, data.angle);
        }
        else
        {
            Svc_Uart_Printf(&svc_interface.uart, "[ENCODER] 发送失败\r\n");
        }

        /* OLED 显示编码器数据 */
        Svc_Oled_Clear(&oled_dev);

        snprintf(buf, sizeof(buf), "Encoder Monitor");
        Svc_Oled_ShowString(&oled_dev, 0, 0, buf);

        snprintf(buf, sizeof(buf), "Cnt:%d Dir:%d", data.count, data.dir);
        Svc_Oled_ShowString(&oled_dev, 0, 2, buf);

        snprintf(buf, sizeof(buf), "Spd:%.1f RPM", data.speed_rpm);
        Svc_Oled_ShowString(&oled_dev, 0, 4, buf);

        snprintf(buf, sizeof(buf), "Ang:%.1f deg", data.angle);
        Svc_Oled_ShowString(&oled_dev, 0, 6, buf);

        Svc_Oled_Refresh(&oled_dev);

        osDelay(1000);
    }
    vTaskDelete(NULL);
}