#include "ringbuffer.h"

void ring_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    // 初始化互斥锁和两个条件变量
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->cond_not_empty, NULL);
    pthread_cond_init(&rb->cond_not_full, NULL);
}

void ring_destroy(RingBuffer *rb)
{
    // 程序退出前释放锁和条件变量的资源
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->cond_not_empty);
    pthread_cond_destroy(&rb->cond_not_full);
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
 * ring_push — 生产者调用，向 buffer 写入一条数据
 *
 * 执行流程：
 *   1. 加锁（如果锁被消费者持有，阻塞等待）
 *   2. 如果 buffer 满了 → cond_wait 释放锁并休眠
 *      被唤醒后 while 会重新检查条件（防止虚假唤醒）
 *   3. 向 head 位置写入数据，head 前移，count +1
 *   4. 发送信号唤醒等待"非空"的消费者
 *   5. 解锁
 */
int ring_push(RingBuffer *rb, MonitorData data)
{
    pthread_mutex_lock(&rb->mutex);
    while (ring_full(rb))
    {
        // cond_wait 做了三件事（原子操作）：
        //   a) 把当前线程加入 cond_not_full 的等待队列
        //   b) 释放 mutex（让消费者可以 pop 腾出空间）
        //   c) 阻塞，直到被 signal 唤醒
        //   被唤醒后会自动重新获取 mutex
        pthread_cond_wait(&rb->cond_not_full, &rb->mutex);
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % BUFFER_SIZE;
    rb->count++;

    // 发送信号：buffer 非空了，唤醒正在等待的消费者
    // signal 只唤醒一个等待线程；broadcast 唤醒全部
    pthread_cond_signal(&rb->cond_not_empty);

    // 解锁：离开临界区
    pthread_mutex_unlock(&rb->mutex);

    return 0;
}

/*
 * ring_pop — 消费者调用，从 buffer 读取一条数据
 *
 * 执行流程与 ring_push 对称：
 *   1. 加锁
 *   2. 如果 buffer 空了 → cond_wait 释放锁并休眠
 *   3. 从 tail 位置取出数据，tail 前移，count -1
 *   4. 发送信号唤醒等待"非满"的生产者
 *   5. 解锁
 */
int ring_pop(RingBuffer *rb, MonitorData *data)
{
    pthread_mutex_lock(&rb->mutex);

    // 等待 buffer 非空
    while (ring_empty(rb))
    {
        // 释放锁 → 休眠 → 被唤醒 → 重新获取锁
        pthread_cond_wait(&rb->cond_not_empty, &rb->mutex);
    }

    // 安全取出数据
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % BUFFER_SIZE;
    rb->count--;

    // 发送信号：buffer 有空位了，唤醒等待的生产者
    pthread_cond_signal(&rb->cond_not_full);

    // 解锁
    pthread_mutex_unlock(&rb->mutex);

    return 0;
}
