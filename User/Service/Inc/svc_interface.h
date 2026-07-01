#ifndef __SVC_INTERFACE_H__
#define __SVC_INTERFACE_H__

#include "svc_uart.h"   //UART服务接口
#include "svc_can.h"    //CAN服务接口    
#include "svc_encoder.h" //编码器服务接口
#include "svc_i2c.h"    //I2C总线服务接口

typedef struct 
{
    svc_uart_t uart;
    svc_can_t can;
    svc_encoder_t encoder;
    svc_i2c_t i2c;        //I2C总线服务
} svc_interface_t;

extern svc_interface_t svc_interface;

void Svc_Interface_Init(void);

#endif