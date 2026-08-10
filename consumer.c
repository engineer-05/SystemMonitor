#include "consumer.h"

#include <stdio.h>

void *consumer_thread(void *arg)
{
    ConsumerArgs *args = (ConsumerArgs *)arg;

    while(1)
    {
        MonitorData data;

        ring_pop(args->buffer,&data);

        printf("[Consumer] CPU: %.2f%%, Mem: %.2f%%, Time: %ld\n",
               data.cpu_usage,data.mem_usage,data.timestamp);
        fflush(stdout);
    }

    return NULL;
}
