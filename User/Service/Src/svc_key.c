#include "svc_key.h"
#include "bsp_key.h"

/**
 * @brief 按键服务初始化
 * @param pIf 服务接口实例
 */
void Svc_Key_InitInstance(svc_key_t *pIf)
{
    Bsp_Key_Init();

    if (pIf->Scan == NULL) {
        pIf->Scan = Bsp_Key_Scan;
    }
}