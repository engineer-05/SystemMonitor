#include "config.h"
#include "producer.h"
#include "consumer.h"
#include "ringbuffer.h"
#include "storage.h"

#include <stdio.h>
#include <signal.h>
#include <pthread.h>

// 线程退出标志，信号处理函数置 0
volatile sig_atomic_t running = 1;

// buffer 设为全局，信号处理函数需要调用 ring_shutdown
static RingBuffer buffer;

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
    ring_shutdown(&buffer);
}

int main()
{
    // 注册信号处理
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGHUP,  signal_handler);

    // ① 连接 MySQL
    MYSQL *conn = storage_init(MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD, MYSQL_DATABASE);
    if (conn == NULL)
        return 1;

    // ② 初始化 RingBuffer
    ring_init(&buffer);

    // ③ 准备线程参数
    ProducerArgs producer_args;
    producer_args.buffer = &buffer;

    ConsumerArgs consumer_args;
    consumer_args.buffer = &buffer;
    consumer_args.conn   = conn;

    // ④ 创建线程
    pthread_t producer_tid;
    pthread_t consumer_tid;

    pthread_create(&producer_tid, NULL, producer_thread, &producer_args);
    pthread_create(&consumer_tid, NULL, consumer_thread, &consumer_args);

    // ⑤ 等待线程结束
    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    // ⑥ 清理
    storage_flush(conn);
    storage_close(conn);
    ring_destroy(&buffer);

    printf("[Main] exit\n");
    return 0;
}
