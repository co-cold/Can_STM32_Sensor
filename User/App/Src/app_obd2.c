#include "app_obd2.h"
#include "svc_interface.h"
#include <string.h>

/* ==================== 驱动场景状态机 ==================== */
typedef enum {
    OBD2_PH_IDLE = 0,      /* 待机：未系安全带→系好→打左转灯 */
    OBD2_PH_IGNITION,      /* 点火起步准备（转速上升） */
    OBD2_PH_ACCEL,         /* 上坡加速 */
    OBD2_PH_BRAKE,         /* 定点制动停车 */
    OBD2_PH_STOPPED,       /* 停车：拉手刹/空挡/报DTC */
    OBD2_PH_HILLSTART,     /* 坡道起步 */
    OBD2_PH_FINISH,        /* 起步完成，循环回待机 */
} obd2_phase_t;

static obd2_vehicle_t g_veh;   /* 当前模拟状态 */
static obd2_phase_t  g_phase;  /* 当前场景阶段 */
static uint32_t      g_phase_ms; /* 当前阶段已运行毫秒 */
static obd2_mode_t   g_mode    = OBD2_MODE_AUTO; /* 工作模式 */
static uint8_t       g_pause   = 0;               /* 暂停自动剧本 */

/* ==================== 车辆模拟 ==================== */
static void Obd2_SimStep(uint32_t dt_ms)
{
    uint32_t t = g_phase_ms;

    /* 暂停：冻结自动剧本（仍继续广播当前值） */
    if (g_pause) return;

    /* 手动模式：脚本不推进，转速随节气门联动，其余值由菜单设定 */
    if (g_mode == OBD2_MODE_MANUAL) {
        g_veh.rpm = 800U + g_veh.throttle * 20U;
        if (g_veh.rpm > 3000U) g_veh.rpm = 3000U;
        return;
    }

    g_phase_ms += dt_ms;

    switch (g_phase)
    {
    case OBD2_PH_IDLE:
        g_veh.rpm = 800;
        g_veh.speed = 0;
        g_veh.throttle = 0;
        g_veh.gear = OBD2_GEAR_P;
        g_veh.brake = 1;
        g_veh.handbrake = 1;
        g_veh.door = 1;
        g_veh.seatbelt = (g_phase_ms > 3000) ? 1 : 0;  /* 约3s后系好安全带 */
        g_veh.left_turn = 0;
        g_veh.dtc_present = 0;
        if (g_phase_ms > 12000) { g_phase_ms = 0; g_phase = OBD2_PH_IGNITION; }
        break;

    case OBD2_PH_IGNITION:
        g_veh.speed = 0;
        g_veh.brake = 1;
        g_veh.handbrake = 1;
        g_veh.gear = OBD2_GEAR_N;
        g_veh.left_turn = 1;                    /* 起步前先开左转向灯 */
        g_veh.throttle = 20;
        g_veh.rpm = 800 + g_phase_ms;           /* 缓慢加油门 */
        if (g_veh.rpm > 1600) g_veh.rpm = 1600;
        if (g_phase_ms > 3000) { g_phase_ms = 0; g_phase = OBD2_PH_ACCEL; }
        break;

    case OBD2_PH_ACCEL:
        g_veh.brake = 0;
        g_veh.handbrake = 0;
        g_veh.gear = OBD2_GEAR_D1;              /* 挂1档上坡 */
        g_veh.throttle = (g_phase_ms < 6000) ? 30 : 40;
        g_veh.speed = (uint8_t)(g_phase_ms / 500);   /* 线性加速到约22km/h */
        if (g_veh.speed > 22) g_veh.speed = 22;
        g_veh.rpm = 1800 + g_phase_ms / 6;
        if (g_veh.rpm > 2600) g_veh.rpm = 2600;
        if (g_phase_ms > 12000) { g_phase_ms = 0; g_phase = OBD2_PH_BRAKE; }
        break;

    case OBD2_PH_BRAKE:
        g_veh.brake = 1;
        g_veh.throttle = 0;
        g_veh.rpm = (g_veh.rpm > 900) ? 1600 - g_phase_ms : 900;
        if (g_veh.rpm < 800) g_veh.rpm = 800;
        g_veh.speed = (g_veh.speed > g_phase_ms / 80) ? (uint8_t)(g_veh.speed - g_phase_ms / 80) : 0;
        if (g_veh.speed == 0 && g_phase_ms > 500) { g_phase_ms = 0; g_phase = OBD2_PH_STOPPED; }
        break;

    case OBD2_PH_STOPPED:
        g_veh.speed = 0;
        g_veh.brake = 0;
        g_veh.gear = OBD2_GEAR_N;               /* 停车挂空挡 */
        g_veh.handbrake = 1;                    /* 拉手刹 */
        g_veh.rpm = 800;
        g_veh.throttle = 0;
        g_veh.left_turn = 1;
        g_veh.dtc_present = 1;                  /* 演示：停车后报出故障码 */
        if (g_phase_ms > 7000) { g_phase_ms = 0; g_phase = OBD2_PH_HILLSTART; }
        break;

    case OBD2_PH_HILLSTART:
        g_veh.brake = 0;
        g_veh.handbrake = (g_phase_ms < 1500) ? 1 : 0;  /* 先松离合再放手刹 */
        g_veh.gear = OBD2_GEAR_D1;              /* 坡起挂1档 */
        g_veh.throttle = 35;
        g_veh.rpm = 1800 + g_phase_ms / 5;
        if (g_veh.rpm > 2400) g_veh.rpm = 2400;
        g_veh.speed = (g_phase_ms > 1500) ? (uint8_t)((g_phase_ms - 1500) / 500) : 0;
        if (g_veh.speed > 18) g_veh.speed = 18;
        g_veh.left_turn = 1;
        if (g_phase_ms > 9000) { g_phase_ms = 0; g_phase = OBD2_PH_FINISH; }
        break;

    case OBD2_PH_FINISH:
        g_veh.speed = 18;
        g_veh.rpm = 1800;
        g_veh.throttle = 30;
        g_veh.left_turn = 0;                    /* 起步完成后关闭转向灯 */
        g_veh.handbrake = 0;
        g_veh.brake = 0;
        g_veh.gear = OBD2_GEAR_D1;
        g_veh.dtc_present = 0;
        if (g_phase_ms > 5000) { g_phase_ms = 0; g_phase = OBD2_PH_IDLE; }
        break;

    default:
        g_phase = OBD2_PH_IDLE;
        break;
    }

    /* 让冷却液温度轻微波动，模拟真实工况 */
    g_veh.coolant = 85 + (HAL_GetTick() / 1000) % 3;
    (void)t; /* 保留 t，便于后续扩展调试 */
}

/* ==================== 广播车身状态（自定义帧） ==================== */
static void Obd2_SendBody(void)
{
    uint8_t basic[3], status[3];

    basic[0] = g_veh.gear;
    basic[1] = g_veh.brake;
    basic[2] = g_veh.handbrake;
    svc_interface.can.Send(OBD2_ID_BODY_BASIC, basic, 3, 20);

    status[0] = g_veh.seatbelt;
    status[1] = g_veh.door;
    status[2] = g_veh.left_turn;
    svc_interface.can.Send(OBD2_ID_BODY_STATUS, status, 3, 20);
}

/* ==================== OBD-II 诊断响应 ==================== */

/* 构造单帧(SF)响应并发送：frame[0]=后续字节数，frame[1..]=payload */
static void Obd2_SendSf(uint32_t id, const uint8_t *payload, uint8_t n)
{
    uint8_t frame[8];
    uint8_t i;

    if (n > 7) n = 7;
    frame[0] = n;
    for (i = 0; i < n; i++) frame[1 + i] = payload[i];
    svc_interface.can.Send(id, frame, n + 1, 20);
}

static void Obd2_HandleRequest(CAN_RxMsg_t *rx)
{
    uint8_t resp[8] = {0};
    uint8_t service;
    uint8_t pid;
    uint8_t n = 0;

    if (rx->len < 2) return;

    /* 仅处理单帧(SF)请求，多帧(FF/CF)暂不支持 */
    if ((rx->data[0] & 0xF0) != 0x00) return;

    service = rx->data[1] & 0x0F;

    switch (service)
    {
    case 0x01: /* Mode 01: 当前动力系统数据 */
        pid = (rx->len > 2) ? rx->data[2] : 0;
        resp[0] = 0x41;
        resp[1] = pid;
        n = 2;
        switch (pid)
        {
        case 0x0C: {                       /* 发动机转速: A*256+B, 转速=rpm/4 */
            uint16_t v = g_veh.rpm / 4;
            resp[2] = (uint8_t)(v >> 8);
            resp[3] = (uint8_t)(v & 0xFF);
            n = 4;
        } break;
        case 0x0D:                          /* 车速 km/h */
            resp[2] = g_veh.speed;
            n = 3;
            break;
        case 0x05:                          /* 冷却液温度: 数值 = ℃ + 40 */
            resp[2] = g_veh.coolant + 40;
            n = 3;
            break;
        case 0x11:                          /* 节气门开度 % */
            resp[2] = g_veh.throttle;
            n = 3;
            break;
        default:                            /* 不支持的 PID → NRC 0x12 请求格式错误 */
            resp[0] = 0x03;
            resp[1] = 0x7F;
            resp[2] = 0x01;
            resp[3] = 0x12;
            n = 4;
            break;
        }
        Obd2_SendSf(OBD2_MSG_ID_DIAG_RESPONSE, resp, n);
        break;

    case 0x03: /* Mode 03: 读取故障码 DTC */
        resp[0] = 0x43;
        if (g_veh.dtc_present) {
            /* 示例故障码 P0100 = 字节 0x01 0x00 */
            resp[1] = 0x01;
            resp[2] = 0x00;
        } else {
            /* 无故障码 */
            resp[1] = 0x00;
            resp[2] = 0x00;
        }
        Obd2_SendSf(OBD2_MSG_ID_DIAG_RESPONSE, resp, 3);
        break;

    default: /* 其他服务暂不响应，忽略 */
        break;
    }
}

/* ==================== 读取当前模拟状态 ==================== */
void Obd2_GetVehicleState(obd2_vehicle_t *v)
{
    if (v != NULL) *v = g_veh;
}

/* ==================== 控制接口 ==================== */
void Obd2_SetMode(obd2_mode_t mode)
{
    g_mode = mode;
    if (mode == OBD2_MODE_AUTO) {   /* 切回自动时从头开始演示 */
        g_phase = OBD2_PH_IDLE;
        g_phase_ms = 0;
    }
}

obd2_mode_t Obd2_GetMode(void)
{
    return g_mode;
}

void Obd2_Pause(int pause)
{
    g_pause = (uint8_t)(pause ? 1 : 0);
}

int Obd2_IsPaused(void)
{
    return (int)g_pause;
}

void Obd2_ResetScenario(void)
{
    g_phase = OBD2_PH_IDLE;
    g_phase_ms = 0;
    g_pause = 0;
    if (g_mode == OBD2_MODE_MANUAL) {
        g_veh.speed = 0;
        g_veh.rpm = 800;
        g_veh.throttle = 0;
        g_veh.gear = OBD2_GEAR_N;
        g_veh.handbrake = 1;
        g_veh.brake = 1;
        g_veh.left_turn = 0;
        g_veh.seatbelt = 0;
        g_veh.dtc_present = 0;
    }
}

void Obd2_SetSpeed(uint8_t kmh)       { g_veh.speed = kmh; }
void Obd2_SetThrottle(uint8_t pct)    { g_veh.throttle = pct; }
void Obd2_SetGear(uint8_t gear)       { g_veh.gear = gear; }
void Obd2_SetHandbrake(uint8_t on)    { g_veh.handbrake = (uint8_t)(on ? 1 : 0); }
void Obd2_SetBrake(uint8_t on)        { g_veh.brake = (uint8_t)(on ? 1 : 0); }
void Obd2_SetLeftTurn(uint8_t on)     { g_veh.left_turn = (uint8_t)(on ? 1 : 0); }
void Obd2_SetSeatbelt(uint8_t on)     { g_veh.seatbelt = (uint8_t)(on ? 1 : 0); }
void Obd2_SetDtc(uint8_t present)     { g_veh.dtc_present = (uint8_t)(present ? 1 : 0); }

/* ==================== 模拟器主任务 ==================== */
void App_Obd2_Task(void *argument)
{
    CAN_RxMsg_t rx;

    Svc_Uart_Printf(&svc_interface.uart, "[OBD2] OBD-II 模拟器任务启动\r\n");

    memset(&g_veh, 0, sizeof(g_veh));
    g_phase = OBD2_PH_IDLE;
    g_phase_ms = 0;

    while (1)
    {
        /* 尽力处理所有已到达的诊断请求（非阻塞） */
        while (svc_interface.can.RecvFrame(&rx, 0) > 0)
        {
            uint32_t id = rx.id;
            /* 功能寻址 0x7DF 或物理寻址 0x7E0-0x7E7 均视为诊断请求 */
            if (id == OBD2_MSG_ID_DIAG_REQUEST || (id >= 0x7E0 && id <= 0x7EF)) {
                Obd2_HandleRequest(&rx);
            }
        }

        /* 推进车辆模拟并广播车身状态帧 */
        Obd2_SimStep(100);
        Obd2_SendBody();

        osDelay(100); /* 100ms 一个发送周期 */
    }

    vTaskDelete(NULL);
}