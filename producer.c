#include "producer.h"
#include "monitor.h"

#include <stdio.h>
#include <unistd.h>

void *producer_thread(void *arg)
{
    ProducerArgs *args = (ProducerArgs *)arg;

    while(1)
    {
        MonitorData data;

        data = collect_monitor_data();

        if(ring_push(args->buffer,data) == 0)
        {
            printf("[Producer] push data\n");
            fflush(stdout);
        }

        usleep(50000);
    }

    return NULL;
}