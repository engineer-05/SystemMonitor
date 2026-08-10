#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "monitor.h"
#include <pthread.h>

#define BUFFER_SIZE 64

typedef struct
{
    MonitorData buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t  cond_not_empty;
    pthread_cond_t  cond_not_full;
}RingBuffer;

//初始化
void ring_init(RingBuffer *rb);

//销毁
void ring_destroy(RingBuffer *rb);

//写入
int ring_push(RingBuffer *rb,MonitorData data);

//读取
int ring_pop(RingBuffer *rb,MonitorData *data);

//判断为空
int ring_empty(RingBuffer *rb);

//判断满
int ring_full(RingBuffer *rb);

#endif