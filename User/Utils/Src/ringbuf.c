#include "ringbuf.h"

/* 内部宏：位运算索引（size 必须为 2 的幂） */
#define RB_MASK(rb) ((rb)->size - 1)

/* 初始化环形缓冲区 */
void RingBuf_Init(RingBuf_t *rb, uint8_t *buf, uint16_t size)
{
    if ((buf == NULL) || (size == 0) || ((size & (size - 1)) != 0)) {
        return;
    }

    rb->buf  = buf;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

/* 写入单字节（ISR安全），成功返回0，满返回-1 */
int RingBuf_Put(RingBuf_t *rb, uint8_t data)
{
    uint16_t next_head = (rb->head + 1) & RB_MASK(rb);

    if (next_head == rb->tail) {
        return -1;
    }

    rb->buf[rb->head] = data;
    rb->head = next_head;
    return 0;
}

/* 写入多字节（ISR安全），返回实际写入的字节数 */
uint16_t RingBuf_Write(RingBuf_t *rb, const uint8_t *data, uint16_t len)
{
    uint16_t free = RingBuf_Free(rb);
    uint16_t mask = RB_MASK(rb);
    uint16_t head = rb->head;

    if (len > free) {
        len = free;
    }
    if (len == 0) {
        return 0;
    }

    uint16_t space_to_end = rb->size - head;

    if (len <= space_to_end) {
        memcpy(&rb->buf[head], data, len);
    } else {
        memcpy(&rb->buf[head], data, space_to_end);
        memcpy(&rb->buf[0], data + space_to_end, len - space_to_end);
    }

    rb->head = (head + len) & mask;
    return len;
}

/* 读取单字节，成功返回0，空返回-1 */
int RingBuf_Get(RingBuf_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail) {
        return -1;
    }

    *data = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) & RB_MASK(rb);
    return 0;
}

/* 读取多字节，返回实际读取的字节数 */
uint16_t RingBuf_Read(RingBuf_t *rb, uint8_t *data, uint16_t len)
{
    uint16_t avail = RingBuf_Available(rb);
    uint16_t mask  = RB_MASK(rb);
    uint16_t tail  = rb->tail;

    if (len > avail) {
        len = avail;
    }
    if (len == 0) {
        return 0;
    }

    uint16_t space_to_end = rb->size - tail;

    if (len <= space_to_end) {
        memcpy(data, &rb->buf[tail], len);
    } else {
        memcpy(data, &rb->buf[tail], space_to_end);
        memcpy(data + space_to_end, &rb->buf[0], len - space_to_end);
    }

    rb->tail = (tail + len) & mask;
    return len;
}

/* 获取未读数据长度 */
uint16_t RingBuf_Available(RingBuf_t *rb)
{
    uint16_t head = rb->head;
    uint16_t tail = rb->tail;

    if (head >= tail) {
        return head - tail;
    } else {
        return rb->size - tail + head;
    }
}

/* 获取剩余空闲空间 */
uint16_t RingBuf_Free(RingBuf_t *rb)
{
    return rb->size - RingBuf_Available(rb) - 1;
}

/* 缓冲区是否为空 */
int RingBuf_IsEmpty(RingBuf_t *rb)
{
    return (rb->head == rb->tail);
}

/* 缓冲区是否已满 */
int RingBuf_IsFull(RingBuf_t *rb)
{
    return (((rb->head + 1) & RB_MASK(rb)) == rb->tail);
}

/* 清空缓冲区 */
void RingBuf_Flush(RingBuf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}