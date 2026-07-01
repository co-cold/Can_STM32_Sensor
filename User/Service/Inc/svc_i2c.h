#ifndef __SVC_I2C_H__
#define __SVC_I2C_H__

#include "cmsis_os.h"

/**
 * @brief I2C 总线服务
 * @note 全局单例，提供 I2C 总线互斥锁，供所有 I2C 设备共享
 */
typedef struct {
    osMutexId_t bus_mutex;  // I2C 总线互斥锁
} svc_i2c_t;

extern svc_i2c_t svc_i2c;

void Svc_I2c_InitInstance(void);

#endif