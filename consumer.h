#ifndef CONSUMER_H
#define CONSUMER_H

#include "ringbuffer.h"
#include <mysql/mysql.h>

typedef struct
{
    RingBuffer *buffer;
    MYSQL      *conn;
}ConsumerArgs;

void *consumer_thread(void *arg);

#endif
