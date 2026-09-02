# Linux 系统监控程序

这是一个使用 C 语言实现的 Linux 系统监控采集端。程序从 `/proc` 读取 CPU 和内存数据，通过生产者—消费者模型送入线程安全的 RingBuffer，并批量写入 MySQL。

> 当前仓库只包含数据采集与 MySQL 存储程序，不包含 Web 服务器、前端仪表盘或截图。Web 展示方案见 `交接文档-Web展示集成.md`；该文档描述的是仓库外的独立项目，不能仅凭本仓库直接运行。

## 已实现功能

- 读取 `/proc/stat`，通过两次采样差值计算整机 CPU 使用率
- 读取 `/proc/meminfo` 的 `MemTotal` 和 `MemAvailable` 计算内存使用率
- Producer 与 Consumer 双线程模型
- 容量为 64 的线程安全 RingBuffer（mutex + condition variable）
- 每累计 10 条数据执行一条多行 `INSERT`
- 处理 `SIGINT`、`SIGTERM`、`SIGQUIT`、`SIGHUP`
- 停止生产后排空 RingBuffer，并将不足 10 条的批量缓存写入数据库
- RingBuffer 正确性与吞吐量测试

## 数据流

```text
/proc/stat + /proc/meminfo
             │
             ▼
      Producer 线程
             │
             ▼
 RingBuffer（容量 64）
             │
             ▼
      Consumer 线程
             │
             ▼
  MySQL monitor_data 表
```

## 环境与依赖

- Linux（依赖 `/proc` 文件系统）
- 支持 C11/POSIX 线程的 GCC 或 Clang
- MySQL Server
- MySQL C 客户端开发库

Ubuntu/Debian 可安装：

```bash
sudo apt install build-essential default-libmysqlclient-dev mysql-server
```

## 数据库初始化

连接参数目前是 [`config.h`](config.h) 中的编译期宏。运行前请按本机环境修改，尤其不要在公开仓库中提交真实生产密码。

```sql
CREATE DATABASE system_monitor;
USE system_monitor;

CREATE TABLE monitor_data (
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    cpu_usage  FLOAT    NOT NULL COMMENT 'CPU 使用率（%）',
    mem_usage  FLOAT    NOT NULL COMMENT '内存使用率（%）',
    timestamp  BIGINT   NOT NULL COMMENT '采集 Unix 时间戳',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

## 编译与运行

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic \
    main.c monitor.c producer.c consumer.c ringbuffer.c storage.c \
    -o main -pthread -lmysqlclient
./main
```

按 `Ctrl+C` 可触发有序退出。程序启动时必须能连接 MySQL，否则会打印错误并返回非零状态。

默认一次循环包含 50 ms 的 CPU 差分采样和 50 ms 的循环等待，因此理论采集周期约为 100 ms，实际周期还包含读取、调度及输出开销。相关宏位于 [`config.h`](config.h)。

## 测试

RingBuffer 测试不依赖 MySQL：

```bash
make -C tests
./tests/ringbuffer_bench 1000000
```

测试会验证消费数量、FIFO 顺序和校验和，并输出吞吐量。性能结果与机器、系统负载、编译选项和 `BUFFER_SIZE` 有关。

## 项目结构

```text
SystemMonitor/
├── main.c                  # 初始化、线程管理、同步等待退出信号、清理
├── monitor.h / monitor.c   # CPU、内存和时间戳采集
├── ringbuffer.h / .c       # 线程安全环形缓冲区
├── producer.h / .c         # 生产者线程
├── consumer.h / .c         # 消费者线程
├── storage.h / .c          # MySQL 连接与批量 INSERT
├── config.h                # 编译期配置
├── tests/                  # RingBuffer 测试与基准程序
└── *.md / *.txt            # 设计、学习与 Web 集成说明
```

## 当前限制

- MySQL 凭据仍硬编码在头文件中，尚未支持运行时配置或环境变量
- 写库失败时当前批次会被丢弃，没有重试、重连或本地文件兜底
- 批量提交只有“10 条”这一条数阈值，没有定时刷写；正常运行时低流量数据可能停留在内存中，退出时才刷新
- SQL 使用多行字符串拼接，不是 `MYSQL_STMT` 预处理，也没有显式事务
- 目前只采集整机 CPU 和内存，不包含磁盘、网络、进程指标或 Web 展示

## 许可证

仓库当前未包含许可证文件。如需以 MIT 许可证发布，请先添加 `LICENSE` 文件；在此之前不应仅凭 README 认定项目已采用 MIT 许可证。
