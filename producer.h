#ifndef PRODUCER_H
#define PRODUCER_H


#include "ringbuffer.h"


typedef struct
{
    RingBuffer *buffer;
}ProducerArgs;

void *producer_thread(void *arg);

#endif