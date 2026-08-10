#ifndef MONITOR_H
#define MONITOR_H


typedef struct
{
    unsigned long long user;        // 用户态时间
    unsigned long long nice;        // 低优先级用户态时间
    unsigned long long system;      // 内核态时间
    unsigned long long idle;        // 空闲时间
    unsigned long long iowait;      // 等待io时间
    unsigned long long irq;
    unsigned long long softirq;

}CPUTime;

typedef struct
{
    // CPU使用率
    float cpu_usage;
    // 内存使用率
    float mem_usage;
    // 采集时间
    long timestamp;

}MonitorData;

CPUTime get_cpu_time();
float get_cpu_usage();
float get_mem_usage();
MonitorData collect_monitor_data();

#endif