#ifndef __APP_HMI_H__
#define __APP_HMI_H__

#include "cmsis_os.h"

/**
 * @brief HMI 菜单任务入口
 * @note 统一管理 OLED + 旋转编码器 + 按键，控制 OBD2 模拟器参数
 */
void App_Hmi_Task(void *argument);

#endif /* __APP_HMI_H__ */