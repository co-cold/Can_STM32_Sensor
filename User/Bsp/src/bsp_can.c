#include "bsp_can.h"
#include "svc_interface.h"

#define CAN_IF (svc_interface.can)

/**
 * @brief CAN 过滤器
 * @param id 过滤器ID
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

/**
 * @brief CAN 初始化
 * @note 该函数负责初始化CAN接口，包括创建RTOS同步对象
 */
void Bsp_Can_Init(void)
{
    /* 创建发送互斥锁 */
    if(CAN_IF.tx_mutex == NULL) {
        const osMutexAttr_t tx_mutex_attr = { .name = "CanTxMute" };
        CAN_IF.tx_mutex = osMutexNew(&tx_mutex_attr);
        if(CAN_IF.tx_mutex == NULL) {
            CAN_IF.ConnectionError = 1;
            return;
        }
    }

    /* 创建发送完成信号量 */
    if(CAN_IF.tx_sem == NULL) {
        const osSemaphoreAttr_t tx_sem_attr = { .name = "CanTxSem" };
        CAN_IF.tx_sem = osSemaphoreNew(1, 0, &tx_sem_attr);
        if(CAN_IF.tx_sem == NULL) {
            CAN_IF.ConnectionError = 1;
            return;
        }
    }

    /* 创建接收消息队列（最多缓存16帧） */
    if(CAN_IF.rx_queue == NULL) {
        CAN_IF.rx_queue = osMessageQueueNew(16, sizeof(CAN_RxMsg_t), NULL);
        if(CAN_IF.rx_queue == NULL) {
            CAN_IF.ConnectionError = 1;
            return;
        }
    }

    /* 启用接收中断 + 发送完成中断 */
    HAL_CAN_ActivateNotification(&hcan,
        CAN_IT_RX_FIFO0_MSG_PENDING |
        CAN_IT_TX_MAILBOX_EMPTY);

    /* 启动CAN外设 */
    if(HAL_CAN_Start(&hcan) != HAL_OK) {
        CAN_IF.ConnectionError = 1;
        return;
    }
}

/**
 * @brief CAN 发送数据帧
 */
int Bsp_Can_Send(uint32_t id, uint8_t *data, uint8_t len, uint32_t timeout_ms)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;
    HAL_StatusTypeDef status;

    if(data == NULL || len == 0 || len > 8) {
        return -1;
    }

    if(CAN_IF.ConnectionError) {
        return -1;
    }

    if(osMutexAcquire(CAN_IF.tx_mutex, timeout_ms) != osOK) {
        return -1;
    }

    osSemaphoreAcquire(CAN_IF.tx_sem, 0);

    /* 构造发送头 */
    tx_header.StdId = id;
    tx_header.ExtId = 0;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = len;
    tx_header.TransmitGlobalTime = DISABLE;

    /* 启动发送 */
    status = HAL_CAN_AddTxMessage(&hcan, &tx_header, data, &tx_mailbox);
    if (status != HAL_OK) {
        osMutexRelease(CAN_IF.tx_mutex);
        return -1;
    }

    /* 等待发送完成信号量 */
    if(osSemaphoreAcquire(CAN_IF.tx_sem, timeout_ms) != osOK) {
        HAL_CAN_AbortTxRequest(&hcan, tx_mailbox);
        osMutexRelease(CAN_IF.tx_mutex);
        return -1;
    }

    /* 发送完成，释放互斥锁 */
    osMutexRelease(CAN_IF.tx_mutex);
    return 0;
}

/**
 * @brief CAN 接收数据帧
 */
int Bsp_Can_RecvFrame(CAN_RxMsg_t *msg, uint32_t timeout_ms)
{
    if(msg == NULL) {
        return -1;
    }

    if(CAN_IF.ConnectionError) {
        return -1;
    }

    if(osMessageQueueGet(CAN_IF.rx_queue, msg, NULL, timeout_ms) != osOK) {
        return -1;
    }

    return 0;
}

/******* HAL 库回调函数 ********/

/**
 * @brief HAL回调：接收完成
 * @note 当CAN接收到新消息时，该回调函数会被调用。它会将接收到的消息放入RTOS消息队列中，供上层应用处理。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    CAN_RxMsg_t msg;

    if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
        msg.id = rx_header.StdId;
        msg.len = rx_header.DLC;
        memcpy(msg.data, rx_data, msg.len);

        if (CAN_IF.rx_queue != NULL) {
            osMessageQueuePut(CAN_IF.rx_queue, &msg, 0, 0);
        }
    }
}

/**
 * @brief HAL回调：发送完成
 * @note 当CAN发送完成后，该回调函数会被调用。它会释放发送完成的信号量，通知等待发送完成的任务。
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if(CAN_IF.tx_sem != NULL) {
        osSemaphoreRelease(CAN_IF.tx_sem);
    }
}

/**
 * @brief HAL回调：发送完成
 * @note 当CAN发送完成后，该回调函数会被调用。它会释放发送完成的信号量，通知等待发送完成的任务。
 */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if(CAN_IF.tx_sem != NULL) {
        osSemaphoreRelease(CAN_IF.tx_sem);
    }
}

/**
 * @brief HAL回调：发送完成
 * @note 当CAN发送完成后，该回调函数会被调用。它会释放发送完成的信号量，通知等待发送完成的任务。
 */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if(CAN_IF.tx_sem != NULL) {
        osSemaphoreRelease(CAN_IF.tx_sem);
    }
}