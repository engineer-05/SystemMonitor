#include "monitor.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main()
{

    while(1)
    {
        MonitorData data;
        data = collect_monitor_data();

        printf("====================\n");
        printf("CPU Usage: %.2f %%\n",data.cpu_usage);
        printf("Memory Usage: %.2f %%\n",data.mem_usage);
        printf("Timestamp: %ld\n",data.timestamp);
        printf("Time: %s",ctime(&data.timestamp));

        sleep(1);
    }

    return 0;
}