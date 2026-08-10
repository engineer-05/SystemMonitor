#include "ringbuffer.h"

void ring_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

int ring_empty(RingBuffer *rb)
{
    return rb->count == 0;
}

int ring_full(RingBuffer *rb)
{
    return rb->count == BUFFER_SIZE;
}

int ring_push(RingBuffer *rb, MonitorData data)
{
    if (ring_full(rb))
    {
        return -1;
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % BUFFER_SIZE;
    rb->count++;

    return 0;
}

int ring_pop(RingBuffer *rb, MonitorData *data)
{
    if (ring_empty(rb))
    {
        return -1;
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % BUFFER_SIZE;
    rb->count--;
    return 0;
}