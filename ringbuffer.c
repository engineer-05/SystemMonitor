#include "ringbuffer.h"

void ring_init(RingBuffer *rb)
{
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
    rb->shutdown = 0;

    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->cond_not_empty, NULL);
    pthread_cond_init(&rb->cond_not_full, NULL);
}

void ring_destroy(RingBuffer *rb)
{
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->cond_not_empty);
    pthread_cond_destroy(&rb->cond_not_full);
}

/*
 * ring_shutdown — 通知 buffer 退出
 *
 *   设置 shutdown 标志，并 broadcast 两个条件变量，
 *   唤醒所有可能正在 cond_wait 的 Producer / Consumer。
 *   信号处理函数调用，只调用一次。
 */
void ring_shutdown(RingBuffer *rb)
{
    pthread_mutex_lock(&rb->mutex);
    rb->shutdown = 1;
    pthread_cond_broadcast(&rb->cond_not_empty);
    pthread_cond_broadcast(&rb->cond_not_full);
    pthread_mutex_unlock(&rb->mutex);
}

int ring_empty(RingBuffer *rb)
{
    return rb->count == 0;
}

int ring_full(RingBuffer *rb)
{
    return rb->count == BUFFER_SIZE;
}

/*
 * ring_push — 生产者写入
 *
 *   返回 0 表示写入成功，返回 -1 表示 shutdown 且 buffer 满，调用者应退出。
 *
 *   退出 while 循环有两种原因：
 *     A) buffer 有空位了 → 正常写入
 *     B) shutdown 信号来了 → 不写，直接返回 -1
 *   所以 while 之后还需要 if 来判断「为什么退出」。
 */
int ring_push(RingBuffer *rb, MonitorData data)
{
    pthread_mutex_lock(&rb->mutex);

    // shutdown 时不必等待，buffer 满也不再等，立刻退出循环
    while (!rb->shutdown && ring_full(rb))
    {
        pthread_cond_wait(&rb->cond_not_full, &rb->mutex);
    }

    // 退出原因 B：shutdown 且 buffer 满 → 放弃写入，返回 -1
    if (rb->shutdown && ring_full(rb))
    {
        pthread_mutex_unlock(&rb->mutex);
        return -1;
    }

    // 退出原因 A：buffer 有空位 → 正常写入
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % BUFFER_SIZE;
    rb->count++;

    pthread_cond_signal(&rb->cond_not_empty);
    pthread_mutex_unlock(&rb->mutex);

    return 0;
}

/*
 * ring_pop — 消费者读取
 *
 *   返回 0 表示读取成功，返回 -1 表示 shutdown 且 buffer 空，调用者应退出。
 *   逻辑与 ring_push 对称：while 退出后仍需 if 判断退出原因。
 */
int ring_pop(RingBuffer *rb, MonitorData *data)
{
    pthread_mutex_lock(&rb->mutex);

    // shutdown 时不必等待，buffer 空也不再等，立刻退出循环
    while (!rb->shutdown && ring_empty(rb))
        pthread_cond_wait(&rb->cond_not_empty, &rb->mutex);

    // 退出原因 B：shutdown 且 buffer 空 → 放弃读取，返回 -1
    if (rb->shutdown && ring_empty(rb))
    {
        pthread_mutex_unlock(&rb->mutex);
        return -1;
    }

    // 退出原因 A：buffer 有数据 → 正常取出
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % BUFFER_SIZE;
    rb->count--;

    pthread_cond_signal(&rb->cond_not_full);
    pthread_mutex_unlock(&rb->mutex);

    return 0;
}
