#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#include "main.h"
#include "tim.h"

uint32_t Bsp_Encoder_GetTick(void);
void Bsp_Encoder_Start(void);
int16_t Bsp_Encoder_GetCount(void);
void Bsp_Encoder_SetCount(int16_t count);

#endif