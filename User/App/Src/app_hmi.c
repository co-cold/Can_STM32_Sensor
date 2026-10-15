#include "app_hmi.h"
#include "app_obd2.h"
#include "svc_interface.h"
#include "svc_oled.h"
#include <stdio.h>

/* ==================== 菜单项定义 ==================== */
typedef enum {
    ITEM_MODE = 0,      /* 自动/手动 */
    ITEM_PAUSE,         /* 运行/暂停 */
    ITEM_RESET,         /* 复位场景 */
    ITEM_SPEED,         /* 手动车速 */
    ITEM_THROTTLE,      /* 手动节气门 */
    ITEM_GEAR,          /* 档位 */
    ITEM_HANDBRAKE,     /* 手刹 */
    ITEM_BRAKE,         /* 刹车 */
    ITEM_LEFTTURN,      /* 左转向灯 */
    ITEM_SEATBELT,      /* 安全带 */
    ITEM_DTC,           /* 故障码 */
    ITEM_COUNT
} menu_item_t;

/* OLED 每页 8 像素高，共 8 页：页0=标题，页1-7=菜单行 */
#define HMI_VISIBLE_ROWS  7

typedef enum {
    HMI_NAV = 0,        /* 滚动选择 */
    HMI_ADJUST,         /* 数值调节 */
} hmi_state_t;

static svc_oled_t  hmi_oled;
static uint8_t     oled_inited = 0;
static int         g_idx    = 0;     /* 当前选中项 */
static int         g_off    = 0;     /* 滚动偏移 */
static hmi_state_t g_state  = HMI_NAV;
static uint8_t     g_adj    = ITEM_SPEED; /* 调节中的项 */
static uint16_t    g_last_cnt = 0;

static const char *gear_name(uint8_t g)
{
    switch (g) {
        case OBD2_GEAR_P:  return "P";
        case OBD2_GEAR_R:  return "R";
        case OBD2_GEAR_N:  return "N";
        case OBD2_GEAR_D1: return "1";
        case OBD2_GEAR_D:  return "D";
        default:           return "P";
    }
}

/* 保证选中项可见 */
static void ClampOffset(void)
{
    if (g_idx < g_off) g_off = g_idx;
    if (g_idx >= g_off + HMI_VISIBLE_ROWS) g_off = g_idx - HMI_VISIBLE_ROWS + 1;
}

/* 生成某一行文本（带未刷新，仅写显存） */
static void BuildLine(int idx, char *buf, size_t sz)
{
    obd2_vehicle_t v;
    const char *cur = (idx == g_idx) ? ">" : " ";
    const char *ON  = "ON", *OFF = "OFF";

    Obd2_GetVehicleState(&v);

    switch (idx) {
    case ITEM_MODE:
        snprintf(buf, sz, "%sMODE:%s", cur, Obd2_GetMode()==OBD2_MODE_AUTO ? "AUTO" : "MANU");
        break;
    case ITEM_PAUSE:
        snprintf(buf, sz, "%sPAUSE:%s", cur, Obd2_IsPaused() ? "PAUS" : "RUN ");
        break;
    case ITEM_RESET:
        snprintf(buf, sz, "%sRESET", cur);
        break;
    case ITEM_SPEED:
        snprintf(buf, sz, "%sSPEED:%3u", cur, (unsigned)v.speed);
        break;
    case ITEM_THROTTLE:
        snprintf(buf, sz, "%sTHROT:%3u", cur, (unsigned)v.throttle);
        break;
    case ITEM_GEAR:
        snprintf(buf, sz, "%sGEAR :%s", cur, gear_name(v.gear));
        break;
    case ITEM_HANDBRAKE:
        snprintf(buf, sz, "%sHBRAK:%s", cur, v.handbrake ? ON : OFF);
        break;
    case ITEM_BRAKE:
        snprintf(buf, sz, "%sBRAKE:%s", cur, v.brake ? ON : OFF);
        break;
    case ITEM_LEFTTURN:
        snprintf(buf, sz, "%sTURN :%s", cur, v.left_turn ? ON : OFF);
        break;
    case ITEM_SEATBELT:
        snprintf(buf, sz, "%sBELT :%s", cur, v.seatbelt ? ON : OFF);
        break;
    case ITEM_DTC:
        snprintf(buf, sz, "%sDTC  :%s", cur, v.dtc_present ? ON : OFF);
        break;
    default:
        buf[0] = 0;
        break;
    }
}

static void Hmi_Render(void)
{
    char buf[20];
    char title[20];
    int row, idx;

    Svc_Oled_Clear(&hmi_oled);

    /* 标题行：模式 + 运行状态 */
    snprintf(title, sizeof(title), "OBD2 %s%s",
        Obd2_GetMode()==OBD2_MODE_AUTO ? "AUTO" : "MANU",
        Obd2_IsPaused() ? " PAUS" : "");
    Svc_Oled_ShowString(&hmi_oled, 0, 0, title);

    /* 菜单行 */
    for (row = 0; row < HMI_VISIBLE_ROWS; row++) {
        idx = g_off + row;
        if (idx >= ITEM_COUNT) break;
        BuildLine(idx, buf, sizeof(buf));
        Svc_Oled_ShowString(&hmi_oled, 0, row + 1, buf);
    }

    Svc_Oled_Refresh(&hmi_oled);
}

/* 激活(进入/切换)某个菜单项 */
static void ActivateItem(int idx)
{
    obd2_vehicle_t v;

    switch (idx) {
    case ITEM_MODE:
        Obd2_SetMode(Obd2_GetMode() == OBD2_MODE_AUTO ? OBD2_MODE_MANUAL : OBD2_MODE_AUTO);
        break;
    case ITEM_PAUSE:
        Obd2_Pause(!Obd2_IsPaused());
        break;
    case ITEM_RESET:
        Obd2_ResetScenario();
        break;
    case ITEM_SPEED:
    case ITEM_THROTTLE:
        g_adj = (uint8_t)idx;
        g_state = HMI_ADJUST;
        break;
    case ITEM_GEAR: {
        Obd2_GetVehicleState(&v);
        uint8_t g = (v.gear == OBD2_GEAR_D) ? OBD2_GEAR_P : (uint8_t)(v.gear + 1);
        Obd2_SetGear(g);
        break;
    }
    case ITEM_HANDBRAKE: Obd2_GetVehicleState(&v); Obd2_SetHandbrake(!v.handbrake); break;
    case ITEM_BRAKE:     Obd2_GetVehicleState(&v); Obd2_SetBrake(!v.brake);     break;
    case ITEM_LEFTTURN:  Obd2_GetVehicleState(&v); Obd2_SetLeftTurn(!v.left_turn); break;
    case ITEM_SEATBELT:  Obd2_GetVehicleState(&v); Obd2_SetSeatbelt(!v.seatbelt); break;
    case ITEM_DTC:       Obd2_GetVehicleState(&v); Obd2_SetDtc(!v.dtc_present);  break;
    default: break;
    }
}

void App_Hmi_Task(void *argument)
{
    obd2_vehicle_t v;
    int16_t delta;
    key_event_t key;

    if (!oled_inited) {
        Svc_Oled_Init(&hmi_oled);
        oled_inited = 1;
    }

    g_idx = 0; g_off = 0; g_state = HMI_NAV;
    g_last_cnt = (uint16_t)svc_interface.encoder.GetData().count;

    Svc_Uart_Printf(&svc_interface.uart, "[HMI] 菜单任务启动\r\n");

    while (1)
    {
        /* 读取输入 */
        delta = (int16_t)(svc_interface.encoder.GetData().count - (int16_t)g_last_cnt);
        g_last_cnt = (uint16_t)svc_interface.encoder.GetData().count;
        key = svc_interface.key.Scan();

        if (g_state == HMI_ADJUST)
        {
            /* 编码器细调数值 */
            if (delta != 0) {
                Obd2_GetVehicleState(&v);
                if (g_adj == ITEM_SPEED) {
                    int s = (int)v.speed + ((delta > 0) ? 1 : -1);
                    if (s < 0) s = 0; if (s > 80) s = 80;
                    Obd2_SetSpeed((uint8_t)s);
                } else if (g_adj == ITEM_THROTTLE) {
                    int t = (int)v.throttle + ((delta > 0) ? 5 : -5);
                    if (t < 0) t = 0; if (t > 100) t = 100;
                    Obd2_SetThrottle((uint8_t)t);
                }
            }
            /* 确认/取消/快捷键 → 返回导航 */
            if (key == KEY_EVENT_SW_SHORT || key == KEY_EVENT_SW_LONG ||
                key == KEY_EVENT_START || key == KEY_EVENT_RESET) {
                g_state = HMI_NAV;
            }
        }
        else /* HMI_NAV */
        {
            if (delta != 0) {
                g_idx += (delta > 0) ? 1 : -1;
                if (g_idx < 0) g_idx = 0;
                if (g_idx >= ITEM_COUNT) g_idx = ITEM_COUNT - 1;
                ClampOffset();
            }
            switch (key) {
            case KEY_EVENT_START:   Obd2_Pause(!Obd2_IsPaused()); break;
            case KEY_EVENT_RESET:   Obd2_ResetScenario(); break;
            case KEY_EVENT_SW_SHORT: ActivateItem(g_idx); break;
            case KEY_EVENT_SW_LONG:  g_idx = 0; g_off = 0; break; /* 返回顶部 */
            default: break;
            }
        }

        Hmi_Render();
        osDelay(50);
    }

    vTaskDelete(NULL);
}