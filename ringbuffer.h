#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "monitor.h"

#define BUFFER_SIZE 5

typedef struct
{
    MonitorData buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;
}RingBuffer;

//初始化
void ring_init(RingBuffer *rb);

//写入
int ring_push(RingBuffer *rb,MonitorData data);

//读取
int ring_pop(RingBuffer *rb,MonitorData *data);

//判断为空
int ring_empty(RingBuffer *rb);

//判断满
int ring_full(RingBuffer *rb);

#endif