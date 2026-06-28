#include "svc_can.h"
#include "bsp_can.h"

/* 当前操作的 CAN 接口实例指针（用于回调中访问 RTOS 对象） */
static svc_can_t *g_can_if = NULL;

/* 当前发送使用的邮箱号（用于超时中止） */
static uint32_t g_can_tx_mailbox = 0;

/**
 * @brief 发送完成回调（注册给 BSP 层）
 * @note 当 CAN 发送完成后，BSP 层调用此回调，释放 tx_sem 信号量
 */
static void Svc_Can_TxComplete(void)
{
    if (g_can_if != NULL && g_can_if->tx_sem != NULL) {
        osSemaphoreRelease(g_can_if->tx_sem);
    }
}

/**
 * @brief 接收完成回调（注册给 BSP 层）
 * @note 当 CAN 接收到新消息时，BSP 层调用此回调，将消息放入 RTOS 消息队列
 */
static void Svc_Can_RxComplete(CAN_RxMsg_t *msg)
{
    if (g_can_if != NULL && g_can_if->rx_queue != NULL) {
        osMessageQueuePut(g_can_if->rx_queue, msg, 0, 0);
    }
}

/**
 * @brief CAN 发送数据帧（带锁）
 * @param id 标准帧ID
 * @param data 发送数据
 * @param len 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @retval >0 发送成功, -1 失败
 */
static int Svc_Can_Send(uint32_t id, uint8_t *data, uint8_t len, uint32_t timeout_ms)
{
    if (data == NULL || len == 0 || len > 8) return -1;
    if (g_can_if == NULL || g_can_if->ConnectionError) return -1;

    if (osMutexAcquire(g_can_if->tx_mutex, timeout_ms) != osOK) {
        return -1;
    }

    osSemaphoreAcquire(g_can_if->tx_sem, 0);

    /* 启动 CAN 发送 */
    if (Bsp_Can_StartTx(id, data, len) != 0) {
        osMutexRelease(g_can_if->tx_mutex);
        return -1;
    }

    /* 等待发送完成信号量 */
    if (osSemaphoreAcquire(g_can_if->tx_sem, timeout_ms) != osOK) {
        Bsp_Can_AbortTx(g_can_tx_mailbox);
        osMutexRelease(g_can_if->tx_mutex);
        return -1;
    }

    osMutexRelease(g_can_if->tx_mutex);
    return (int)len;
}

/**
 * @brief CAN 接收数据帧
 * @param msg 接收消息结构体指针
 * @param timeout_ms 超时时间（毫秒）
 * @retval >0 接收成功（返回数据长度）, -1 失败
 */
static int Svc_Can_RecvFrame(CAN_RxMsg_t *msg, uint32_t timeout_ms)
{
    if (msg == NULL) return -1;
    if (g_can_if == NULL || g_can_if->ConnectionError) return -1;

    if (osMessageQueueGet(g_can_if->rx_queue, msg, NULL, timeout_ms) != osOK) {
        return -1;
    }

    return (int)(msg->len);
}

/**
 * @brief CAN 服务接口初始化
 * @note 挂载BSP函数指针，配置默认过滤器
 */
void Svc_Can_InitInstance(svc_can_t *pIf)
{
    g_can_if = pIf;

    /* 创建发送互斥锁 */
    if (pIf->tx_mutex == NULL) {
        const osMutexAttr_t tx_mutex_attr = { .name = "CanTxMute" };
        pIf->tx_mutex = osMutexNew(&tx_mutex_attr);
        if (pIf->tx_mutex == NULL) {
            pIf->ConnectionError = 1;
            return;
        }
    }

    /* 创建发送完成信号量 */
    if (pIf->tx_sem == NULL) {
        const osSemaphoreAttr_t tx_sem_attr = { .name = "CanTxSem" };
        pIf->tx_sem = osSemaphoreNew(1, 0, &tx_sem_attr);
        if (pIf->tx_sem == NULL) {
            pIf->ConnectionError = 1;
            return;
        }
    }

    /* 创建接收消息队列（最多缓存16帧） */
    if (pIf->rx_queue == NULL) {
        pIf->rx_queue = osMessageQueueNew(16, sizeof(CAN_RxMsg_t), NULL);
        if (pIf->rx_queue == NULL) {
            pIf->ConnectionError = 1;
            return;
        }
    }

    /* 注册回调到 BSP 层 */
    Bsp_Can_RegisterTxCpltCallback(Svc_Can_TxComplete);
    Bsp_Can_RegisterRxCpltCallback(Svc_Can_RxComplete);

    /* 配置默认过滤器：接收所有标准帧 */
    Bsp_Can_ConfigFilter(0x000, 0x000);

    /* 启动 CAN 外设 */
    if (Bsp_Can_Start() != 0) {
        pIf->ConnectionError = 1;
        return;
    }

    /* 挂载函数指针 */
    pIf->Send       = Svc_Can_Send;
    pIf->RecvFrame  = Svc_Can_RecvFrame;
}