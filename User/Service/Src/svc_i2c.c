#include "svc_i2c.h"

/**
 * @brief I2C 总线服务全局实例
 * @note 所有 I2C 设备（OLED、AHT21、EEPROM 等）共用此总线锁
 */
svc_i2c_t svc_i2c;

/**
 * @brief I2C 总线服务初始化
 * @note 创建总线互斥锁，确保同一时间只有一个设备操作 I2C 总线
 */
void Svc_I2c_InitInstance(void)
{
    if (svc_i2c.bus_mutex == NULL) {
        const osMutexAttr_t mutex_attr = { .name = "I2cBusMutex" };
        svc_i2c.bus_mutex = osMutexNew(&mutex_attr);
    }
}