#include "svc_can.h"
#include "bsp_can.h"

/**
 * @brief CAN 服务接口初始化
 * @note 挂载BSP函数指针，配置默认过滤器
 */
void Svc_Can_InitInstance(svc_can_t *pIf)
{
    /* 挂载函数指针（和UART写法完全一致） */
    pIf->Init       = Bsp_Can_Init;
    pIf->Send       = Bsp_Can_Send;
    pIf->RecvFrame  = Bsp_Can_RecvFrame;

    /* 调用BSP初始化（创建RTOS对象、启动CAN） */
    pIf->Init();

    /* 配置默认过滤器：接收所有标准帧 */
    Bsp_Can_ConfigFilter(0x000, 0x000);
}