#include "svc_encoder.h"
#include "bsp_encoder.h"
#include "svc_interface.h"

#define ENCODER_IF (svc_interface.encoder)

/**
 * @brief 更新编码器数据
 * @note 该函数读取当前计数和时间，计算总计数和转速，并处理16位计数器的溢出情况。
 */
static void Svc_Encoder_Update(void)
{
    Encoder_State_t *s = &ENCODER_IF.state;
    int16_t current_count = Bsp_Encoder_GetCount();
    uint32_t current_time = HAL_GetTick();

    if(s->prev_time == 0) {
        s->prev_time = current_time;
        s->prev_count = current_count;
        s->total_count = current_count;
        s->speed_rpm = 0.0f;
        return;
    }

    int16_t diff = current_count - s->prev_count;
    uint32_t delta_time = current_time - s->prev_time;

    /* 处理16位溢出 */
    if(diff > 32767) diff -= 65536;
    if(diff < -32768) diff += 65536;

    s->total_count += diff;

    if (delta_time > 0 && diff != 0) {
        float delta_time_sec = (float)delta_time / 1000.0f;
        s->speed_rpm = ((float)diff / s->counts_per_rev) * (60.0f / delta_time_sec);
    } else {
        s->speed_rpm = 0.0f;
    }

    s->prev_count = current_count;
    s->prev_time = current_time;
}

/**
 * @brief 获取编码器数据
 * @note 该函数调用 Update() 更新数据，然后返回当前计数、转速、角度和方向。
 */
static Encoder_Data_t Svc_Encoder_GetData(void)
{
    Svc_Encoder_Update();

    Encoder_Data_t data;
    Encoder_State_t *s = &ENCODER_IF.state;

    data.count = (int16_t)(s->total_count & 0xFFFF);
    data.speed_rpm = s->speed_rpm;
    data.angle = ((s->total_count % s->counts_per_rev) / (float)s->counts_per_rev) * 360.0f;

    if (s->total_count > (s->prev_count & 0xFFFF))
        data.dir = 1;
    else if (s->total_count < (s->prev_count & 0xFFFF))
        data.dir = -1;
    else
        data.dir = 0;

    return data;
}

/**
 * @brief 获取转速 RPM
 * @note 直接返回当前计算的转速值。
 */
static float Svc_Encoder_GetSpeedRpm(void)
{
    return ENCODER_IF.state.speed_rpm;
}

/**
 * @brief 获取角度
 * @note 根据总计数和每转脉冲数计算当前角度，范围为0-360度。
 */
static float Svc_Encoder_GetAngle(void)
{
    Encoder_State_t *s = &ENCODER_IF.state;
    return ((s->total_count % s->counts_per_rev) / (float)s->counts_per_rev) * 360.0f;
}

/**
 * @brief 重置计数
 * @note 该函数将总计数、上一次计数和时间重置为0，并将转速设置为0，同时调用BSP函数将硬件计数器清零。
 */
static void Svc_Encoder_ResetCount(void)
{
    Encoder_State_t *s = &ENCODER_IF.state;
    s->total_count = 0;
    s->prev_count = 0;
    s->prev_time = 0;
    s->speed_rpm = 0.0f;
    Bsp_Encoder_SetCount(0);
}

/**
 * @brief 编码器服务接口实例初始化
 * @note 该函数挂载接口函数指针，初始化状态，并启动编码器硬件。
 */
void Svc_Encoder_InitInstance(svc_encoder_t *pIf, uint16_t counts_per_rev)
{
    /* 挂载函数指针 */
    pIf->Init       = Bsp_Encoder_Start;
    pIf->Update     = Svc_Encoder_Update;
    pIf->GetData    = Svc_Encoder_GetData;
    pIf->GetSpeedRpm = Svc_Encoder_GetSpeedRpm;
    pIf->GetAngle   = Svc_Encoder_GetAngle;
    pIf->ResetCount = Svc_Encoder_ResetCount;

    /* 初始化状态 */
    pIf->state.total_count = 0;
    pIf->state.prev_count = 0;
    pIf->state.prev_time = 0;
    pIf->state.speed_rpm = 0.0f;
    pIf->state.counts_per_rev = counts_per_rev;

    /* 启动编码器硬件 */
    pIf->Init();
}