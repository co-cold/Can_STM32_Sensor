#include "bsp_encoder.h"

void Bsp_Encoder_Start(void)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
}

int16_t Bsp_Encoder_GetCount(void)
{
    return (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
}

void Bsp_Encoder_SetCount(int16_t count)
{
    __HAL_TIM_SET_COUNTER(&htim3, count);
}