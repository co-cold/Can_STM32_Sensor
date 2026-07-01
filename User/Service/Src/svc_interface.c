#include "svc_interface.h"

#include "svc_uart.h"
#include "svc_can.h"
#include "svc_encoder.h"
#include "svc_i2c.h"

/* 定义全局接口实例 */
svc_interface_t svc_interface;

/**
 * @brief 初始化硬件接口
 * @note 该函数负责初始化所有硬件接口实例。
 */
void Svc_Interface_Init(void)
{
    Svc_Uart_InitInstance(&svc_interface.uart);
    Svc_Can_InitInstance(&svc_interface.can);
    Svc_Encoder_InitInstance(&svc_interface.encoder, 1024);
    Svc_I2c_InitInstance();
}