#include "consumer.h"
#include "storage.h"

#include <signal.h>
#include <stdio.h>

extern volatile sig_atomic_t running;

void *consumer_thread(void *arg)
{
    ConsumerArgs *args = (ConsumerArgs *)arg;

    while(running)
    {
        MonitorData data;

        if(ring_pop(args->buffer,&data) != 0)
        {
            // pop 失败（buffer shutdown），退出
            break;
        }

        printf("[Consumer] CPU: %.2f%%, Mem: %.2f%%, Time: %ld\n",
               data.cpu_usage,data.mem_usage,data.timestamp);
        fflush(stdout);

        storage_save(args->conn,&data);
    }

    printf("[Consumer] exit\n");
    return NULL;
}
