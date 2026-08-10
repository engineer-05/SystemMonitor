#include "producer.h"
#include "consumer.h"
#include "ringbuffer.h"
#include "storage.h"

#include <stdio.h>
#include <pthread.h>


int main()
{
    // ① 连接 MySQL
    MYSQL *conn = storage_init("localhost","lzx","123456","system_monitor");
    if (conn == NULL)
    {
        return 1;
    }

    // ② 初始化 RingBuffer
    RingBuffer buffer;
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

    pthread_create(&producer_tid,NULL,producer_thread,&producer_args);
    pthread_create(&consumer_tid,NULL,consumer_thread,&consumer_args);

    pthread_join(producer_tid,NULL);
    pthread_join(consumer_tid,NULL);

    // ⑤ 清理
    storage_flush(conn);
    storage_close(conn);
    ring_destroy(&buffer);

    return 0;
}