#ifndef __APP_OBD2_H__
#define __APP_OBD2_H__

#include "cmsis_os.h"
#include <stdint.h>

/* ==================== OBD-II CAN ID 定义 ==================== */

/* 诊断请求/响应（标准 OBD-II，SAE J1979 地址） */
#define OBD2_MSG_ID_DIAG_REQUEST   0x7DFU  /* i.MX6ULL 下发的诊断请求（功能寻址） */
#define OBD2_MSG_ID_DIAG_RESPONSE  0x7E8U  /* 本机作为 ECU 的响应地址 */

/* 车身状态：无标准 PID，由模拟器以自定义 CAN ID 周期性广播（供 SocketCAN 解析） */
#define OBD2_ID_BODY_BASIC   0x360U  /* D[0]=档位  D[1]=刹车  D[2]=手刹 */
#define OBD2_ID_BODY_STATUS  0x361U  /* D[0]=安全带  D[1]=车门  D[2]=左转向灯 */

/* ==================== 档位编码（自定义帧） ==================== */
#define OBD2_GEAR_P  0  /* P 驻车档 */
#define OBD2_GEAR_R  1  /* R 倒档   */
#define OBD2_GEAR_N  2  /* N 空挡   */
#define OBD2_GEAR_D1 3  /* 1档(坡道起步档) */
#define OBD2_GEAR_D  4  /* D 前进档 */

/* ==================== 车载状态结构体 ==================== */
typedef struct {
    uint8_t  gear;        /* 档位 */
    uint8_t  brake;       /* 0=松开 1=踩下 */
    uint8_t  handbrake;   /* 0=放下 1=拉起 */
    uint8_t  seatbelt;    /* 0=未系 1=已系 */
    uint8_t  door;        /* 0=开  1=关 */
    uint8_t  left_turn;   /* 0=关  1=左转向灯开 */
    uint16_t rpm;         /* 发动机转速 rpm */
    uint8_t  speed;       /* 车速 km/h */
    uint8_t  throttle;    /* 节气门开度 % */
    uint8_t  coolant;     /* 冷却液温度 ℃ */
    uint8_t  dtc_present; /* 0=无故障 1=存在故障码 */
} obd2_vehicle_t;

/* ==================== 工作模式 ==================== */
typedef enum {
    OBD2_MODE_AUTO = 0,   /* 自动坡道演示（固定剧本） */
    OBD2_MODE_MANUAL,     /* 手动控制（编码器/菜单设定参数） */
} obd2_mode_t;

/* ==================== 对外接口 ==================== */
void App_Obd2_Task(void *argument);          /* 模拟器任务入口 */
void Obd2_GetVehicleState(obd2_vehicle_t *v);/* 读取当前模拟状态 */

/* 控制接口（供 HMI 菜单任务调用） */
void Obd2_SetMode(obd2_mode_t mode);         /* 切换自动/手动 */
obd2_mode_t Obd2_GetMode(void);
void Obd2_Pause(int pause);                  /* 1=暂停自动剧本, 0=继续 */
int  Obd2_IsPaused(void);
void Obd2_ResetScenario(void);               /* 复位场景到待机 */
void Obd2_SetSpeed(uint8_t kmh);             /* 手动设定车速 */
void Obd2_SetThrottle(uint8_t pct);          /* 手动设定节气门 */
void Obd2_SetGear(uint8_t gear);             /* 手动设定档位 */
void Obd2_SetHandbrake(uint8_t on);          /* 手动设定手刹 */
void Obd2_SetBrake(uint8_t on);              /* 手动设定刹车 */
void Obd2_SetLeftTurn(uint8_t on);           /* 手动设定左转向灯 */
void Obd2_SetSeatbelt(uint8_t on);           /* 手动设定安全带 */
void Obd2_SetDtc(uint8_t present);           /* 手动设定故障码 */

#endif /* __APP_OBD2_H__ */