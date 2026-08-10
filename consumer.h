#ifndef CONSUMER_H
#define CONSUMER_H

#include "ringbuffer.h"

typedef struct
{
    RingBuffer *buffer;
}ConsumerArgs;

void *consumer_thread(void *arg);

#endif
