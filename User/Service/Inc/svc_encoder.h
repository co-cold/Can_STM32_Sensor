#ifndef __SVC_ENCODER_H__
#define __SVC_ENCODER_H__

#include <stdint.h>
#include "cmsis_os.h"

/* 编码器数据结构 */
typedef struct {
    int32_t total_count;  // 总计数
    int16_t prev_count;   // 上一次计数
    uint32_t prev_time;   // 上一次时间
    float    speed_rpm;   // 当前转速
    uint16_t counts_per_rev; // 每转脉冲数
} Encoder_State_t;

typedef struct {
    int16_t  count;   // 当前计数（低16位）
    int8_t   dir;     // 方向
    float    speed_rpm;  // 转速 RPM
    float    angle;    // 角度 0-360
} Encoder_Data_t;

/* 编码器服务接口 */
typedef struct
{
    uint8_t ConnectionError;    // 初始化错误标记

    /* 编码器状态 */
    Encoder_State_t state;

    /* 接口函数 */
    void (*Init)(void);
    void (*Update)(void);
    Encoder_Data_t (*GetData)(void);
    float (*GetSpeedRpm)(void);
    float (*GetAngle)(void);
    void (*ResetCount)(void);
} svc_encoder_t;

void Svc_Encoder_InitInstance(svc_encoder_t *pIf, uint16_t counts_per_rev);

#endif