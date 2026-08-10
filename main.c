#include "producer.h"
#include "consumer.h"
#include "ringbuffer.h"

#include <stdio.h>
#include <pthread.h>


int main()
{
    RingBuffer buffer;

    ring_init(&buffer);

    ProducerArgs producer_args;
    producer_args.buffer = &buffer;

    ConsumerArgs consumer_args;
    consumer_args.buffer = &buffer;

    pthread_t producer_tid;
    pthread_t consumer_tid;

    pthread_create(&producer_tid,NULL,producer_thread,&producer_args);
    pthread_create(&consumer_tid,NULL,consumer_thread,&consumer_args);

    pthread_join(producer_tid,NULL);
    pthread_join(consumer_tid,NULL);

    return 0;
}