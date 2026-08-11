#include "producer.h"
#include "monitor.h"
#include "config.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

extern volatile sig_atomic_t running;

void *producer_thread(void *arg)
{
    ProducerArgs *args = (ProducerArgs *)arg;

    while(running)
    {
        MonitorData data;

        data = collect_monitor_data();

        if(ring_push(args->buffer,data) == 0)
        {
            printf("[Producer] push data\n");
            fflush(stdout);
        }
        else
        {
            // push 失败（buffer shutdown），退出
            break;
        }

        usleep(LOOP_INTERVAL_US);
    }

    printf("[Producer] exit\n");
    return NULL;
}
