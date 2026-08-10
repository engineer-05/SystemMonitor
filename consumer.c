#include "consumer.h"

#include <stdio.h>
#include <unistd.h>

void *consumer_thread(void *arg)
{
    ConsumerArgs *args = (ConsumerArgs *)arg;

    while(1)
    {
        MonitorData data;

        if(ring_pop(args->buffer,&data) == 0)
        {
            printf("[Consumer] CPU: %.2f%%, Mem: %.2f%%, Time: %ld\n",
                   data.cpu_usage,data.mem_usage,data.timestamp);
            fflush(stdout);
        }
        else
        {
            usleep(100000);
        }
    }

    return NULL;
}
