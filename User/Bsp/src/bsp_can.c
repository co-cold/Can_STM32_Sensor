#include "bsp_can.h"
#include <string.h>

/* 回调函数指针 */
static Bsp_Can_TxCpltCallback tx_cplt_cb = NULL;
static Bsp_Can_RxCpltCallback rx_cplt_cb = NULL;

/**
 * @brief 注册发送完成回调
 * @param cb 回调函数指针
 */
void Bsp_Can_RegisterTxCpltCallback(Bsp_Can_TxCpltCallback cb)
{
    tx_cplt_cb = cb;
}

/**
 * @brief 注册接收完成回调
 * @param cb 回调函数指针
 */
void Bsp_Can_RegisterRxCpltCallback(Bsp_Can_RxCpltCallback cb)
{
    rx_cplt_cb = cb;
}

/**
 * @brief 启动 CAN 外设
 * @note 启用接收中断和发送完成中断，然后启动CAN外设
 * @retval 0 成功, -1 失败
 */
int Bsp_Can_Start(void)
{
    /* 启用接收中断 + 发送完成中断 */
    HAL_CAN_ActivateNotification(&hcan,
        CAN_IT_RX_FIFO0_MSG_PENDING |
        CAN_IT_TX_MAILBOX_EMPTY);

    /* 启动CAN外设 */
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief 启动 CAN 发送
 * @param id 标准帧ID
 * @param data 发送数据
 * @param len 数据长度
 * @retval 0 成功, -1 失败
 */
int Bsp_Can_StartTx(uint32_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;

    if (data == NULL || len == 0 || len > 8) {
        return -1;
    }

    /* 构造发送头 */
    tx_header.StdId = id;
    tx_header.ExtId = 0;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = len;
    tx_header.TransmitGlobalTime = DISABLE;

    /* 启动发送 */
    if (HAL_CAN_AddTxMessage(&hcan, &tx_header, data, &tx_mailbox) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief 中止 CAN 发送
 * @param tx_mailbox 发送邮箱号
 */
void Bsp_Can_AbortTx(uint32_t tx_mailbox)
{
    HAL_CAN_AbortTxRequest(&hcan, tx_mailbox);
}

/**
 * @brief CAN 过滤器配置
 * @param id 过滤器ID
 * @param mask 过滤器掩码
 */
void Bsp_Can_ConfigFilter(uint32_t id, uint32_t mask)
{
    CAN_FilterTypeDef sFilterConfig = {0};
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = (id << 5) & 0xFFE0;
    sFilterConfig.FilterIdLow = (id << 5) & 0xFFE0;
    sFilterConfig.FilterMaskIdHigh = (mask << 5) & 0xFFE0;
    sFilterConfig.FilterMaskIdLow = (mask << 5) & 0xFFE0;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);
}

/******* HAL 库回调函数 ********/

/**
 * @brief HAL回调：接收完成
 * @note 当CAN接收到新消息时，该回调函数会被调用。它会通过回调函数指针通知上层。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    CAN_RxMsg_t msg;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
        msg.id = rx_header.StdId;
        msg.len = rx_header.DLC;
        memcpy(msg.data, rx_data, msg.len);

        if (rx_cplt_cb != NULL) {
            rx_cplt_cb(&msg);
        }
    }
}

/**
 * @brief HAL回调：发送完成
 * @note 当CAN发送完成后，该回调函数会被调用。它会通过回调函数指针通知上层。
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (tx_cplt_cb != NULL) {
        tx_cplt_cb();
    }
}

/**
 * @brief HAL回调：发送完成
 * @note 当CAN发送完成后，该回调函数会被调用。它会通过回调函数指针通知上层。
 */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (tx_cplt_cb != NULL) {
        tx_cplt_cb();
    }
}

/**
 * @brief HAL回调：发送完成
 * @note 当CAN发送完成后，该回调函数会被调用。它会通过回调函数指针通知上层。
 */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (tx_cplt_cb != NULL) {
        tx_cplt_cb();
    }
}