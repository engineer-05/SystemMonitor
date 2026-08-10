#include "producer.h"
#include "ringbuffer.h"

#include <stdio.h>
#include <pthread.h>


int main()
{
    RingBuffer buffer;

    ring_init(&buffer);

    ProducerArgs args;
    args.buffer = &buffer;

    pthread_t tid;

    pthread_create(&tid,NULL,producer_thread,&args);

    pthread_join(tid,NULL);

    return 0;
}