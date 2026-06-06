
#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* 环形缓冲区结构 */
typedef struct {
    uint8_t *buf;   /* 数据缓冲区 */
    uint16_t size;  /* 缓冲区总大小（必须为2的幂） */
    volatile uint16_t head;  /* 写指针（ISR更新） */
    volatile uint16_t tail;  /* 读指针（应用层更新） */
} RingBuf_t;

/* 初始化环形缓冲区 */
void RingBuf_Init(RingBuf_t *rb, uint8_t *buf, uint16_t size);

/* 写入单字节（ISR安全），成功返回0，满返回-1 */
int RingBuf_Put(RingBuf_t *rb, uint8_t data);

/* 写入多字节（ISR安全），返回实际写入的字节数 */
uint16_t RingBuf_Write(RingBuf_t *rb, const uint8_t *data, uint16_t len);

/* 读取单字节，成功返回0，空返回-1 */
int RingBuf_Get(RingBuf_t *rb, uint8_t *data);

/* 读取多字节，返回实际读取的字节数 */
uint16_t RingBuf_Read(RingBuf_t *rb, uint8_t *data, uint16_t len);

/* 获取未读数据长度 */
uint16_t RingBuf_Available(RingBuf_t *rb);

/* 获取剩余空闲空间 */
uint16_t RingBuf_Free(RingBuf_t *rb);

/* 缓冲区是否为空 */
int RingBuf_IsEmpty(RingBuf_t *rb);

/* 缓冲区是否已满 */
int RingBuf_IsFull(RingBuf_t *rb);

/* 清空缓冲区 */
void RingBuf_Flush(RingBuf_t *rb);

#endif