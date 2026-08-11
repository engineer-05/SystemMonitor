#include "monitor.h"
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>

CPUTime get_cpu_time()
{

    CPUTime cpu = {0};
    FILE *fp = fopen("/proc/stat", "r");

    if (fp == NULL)
    {
        perror("open /proc/stat");
        return cpu;
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);

    sscanf(buffer,

           "cpu %llu %llu %llu %llu %llu %llu %llu",
           &cpu.user,
           &cpu.nice,
           &cpu.system,
           &cpu.idle,
           &cpu.iowait,
           &cpu.irq,
           &cpu.softirq);

    fclose(fp);
    return cpu;
}

float get_cpu_usage()
{

    CPUTime start;
    CPUTime end;

    start = get_cpu_time();
    usleep(CPU_INTERVAL_US);
    end = get_cpu_time();

    unsigned long long start_total =
        start.user +
        start.nice +
        start.system +
        start.idle +
        start.iowait +
        start.irq +
        start.softirq;

    unsigned long long end_total =
        end.user +
        end.nice +
        end.system +
        end.idle +
        end.iowait +
        end.irq +
        end.softirq;

    // CPU 总共经过的时间
    unsigned long long total_diff = end_total - start_total;
    // CPU 空闲时间增加了多少
    unsigned long long idle_diff = (end.idle + end.iowait) - (start.idle + start.iowait);

    // 避免除 0 错误，如果两次采样期间 CPU 总时间没有增长，说明没有有效数据，直接返回0
    if(total_diff==0)
        return 0;

    // CPU使用率 = (总时间 - 空闲时间) / 总时间
    return (float)(total_diff-idle_diff) / total_diff * 100;

}

float get_mem_usage()
{

    FILE *fp;
    fp=fopen("/proc/meminfo","r");

    if(fp==NULL)
    {
        perror("open /proc/meminfo");
        return 0;
    }

    char buffer[256];
    unsigned long long mem_total=0;
    unsigned long long mem_available=0;

    // 读 buffer 一次读一行，读完就退出
    while(fgets(buffer,sizeof(buffer),fp))
    {
        // 从字符串里面按照格式提取数据
        if(sscanf(buffer,"MemTotal: %llu",&mem_total)==1)
        {

        }
        // 从字符串里面按照格式提取数据
        if(sscanf(buffer,"MemAvailable: %llu",&mem_available)==1)
        {

        }

        if(mem_total && mem_available)
        {
            break;
        }
    }

    fclose(fp);

    if(mem_total==0)
        return 0;

    float usage = (float)(mem_total-mem_available) / mem_total * 100;
    return usage;
}

MonitorData collect_monitor_data()
{

    MonitorData data;

    data.cpu_usage = get_cpu_usage();
    data.mem_usage = get_mem_usage();
    data.timestamp = time(NULL);

    return data;
}