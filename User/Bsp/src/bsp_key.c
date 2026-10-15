#include "bsp_key.h"
#include "main.h"

/* ==================== 按键引脚定义 ==================== */
/* SW: 编码器自带按键   PB0 */
#define KEY_SW_GPIO_Port     GPIOB
#define KEY_SW_Pin           GPIO_PIN_0

/* KEY_A: 开始/暂停      PB1（空闲引脚，需按实际硬件接线调整） */
#define KEY_START_GPIO_Port  GPIOB
#define KEY_START_Pin        GPIO_PIN_1

/* KEY_B: 复位           PB10（空闲引脚，需按实际硬件接线调整） */
#define KEY_RESET_GPIO_Port  GPIOB
#define KEY_RESET_Pin        GPIO_PIN_10

#define KEY_PRESSED          0   /* 按键按下时读数为 0（上拉） */
#define KEY_LONG_MS          1000 /* 长按判定阈值 */

static uint8_t sw_raw = 1, sw_cnt = 0, sw_held = 0;
static uint32_t sw_press_t = 0;

static uint8_t start_raw = 1, start_cnt = 0, start_pressed = 0;
static uint8_t reset_raw = 1, reset_cnt = 0, reset_pressed = 0;

/**
 * @brief 初始化按键 GPIO（自包含，不依赖 MX_GPIO_Init，CubeMX 重新生成也不会丢失）
 */
void Bsp_Key_Init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    g.Pin  = KEY_SW_Pin | KEY_START_Pin | KEY_RESET_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &g);
}

/**
 * @brief 按键轮询扫描（约每 10ms 调用一次）
 * @retval 按键事件
 */
key_event_t Bsp_Key_Scan(void)
{
    key_event_t ev = KEY_EVENT_NONE;
    uint32_t now = HAL_GetTick();
    uint8_t sw, start, reset;

    /* 该函数可能被多任务调用，但服务层已加锁，这里仅由 HMI 任务调用 */
    sw    = (HAL_GPIO_ReadPin(KEY_SW_GPIO_Port, KEY_SW_Pin) == GPIO_PIN_RESET) ? KEY_PRESSED : 1;
    start = (HAL_GPIO_ReadPin(KEY_START_GPIO_Port, KEY_START_Pin) == GPIO_PIN_RESET) ? KEY_PRESSED : 1;
    reset = (HAL_GPIO_ReadPin(KEY_RESET_GPIO_Port, KEY_RESET_Pin) == GPIO_PIN_RESET) ? KEY_PRESSED : 1;

    /* ---- 编码器按键 SW：区分短按/长按 ---- */
    if (sw != sw_raw) { sw_raw = sw; sw_cnt = 1; }
    else if (sw_cnt < 2) { sw_cnt++; }
    else { /* 抖动已稳定 */
        if (sw_raw == KEY_PRESSED) {
            if (!sw_held) { sw_held = 1; sw_press_t = now; }
        }
        else {
            if (sw_held) {
                if (now - sw_press_t >= KEY_LONG_MS) ev = KEY_EVENT_SW_LONG;
                else ev = KEY_EVENT_SW_SHORT;
                sw_held = 0;
            }
        }
    }

    /* ---- KEY_A：开始/暂停（按下沿触发） ---- */
    if (start != start_raw) { start_raw = start; start_cnt = 1; }
    else if (start_cnt < 2) { start_cnt++; }
    else {
        if (start_raw == KEY_PRESSED && !start_pressed) {
            start_pressed = 1;
            ev = KEY_EVENT_START;
        }
        else if (start_raw != KEY_PRESSED && start_pressed) {
            start_pressed = 0;
        }
    }

    /* ---- KEY_B：复位（按下沿触发） ---- */
    if (reset != reset_raw) { reset_raw = reset; reset_cnt = 1; }
    else if (reset_cnt < 2) { reset_cnt++; }
    else {
        if (reset_raw == KEY_PRESSED && !reset_pressed) {
            reset_pressed = 1;
            ev = KEY_EVENT_RESET;
        }
        else if (reset_raw != KEY_PRESSED && reset_pressed) {
            reset_pressed = 0;
        }
    }

    return ev;
}