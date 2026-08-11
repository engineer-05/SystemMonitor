# Linux 系统监控程序

C 语言从零实现的 Linux 系统监控程序，实时采集 CPU 和内存使用率，通过生产者-消费者模型批量写入 MySQL，配套 Web 仪表盘可视化展示。

## 项目展示

启动后浏览器访问 `http://<主机IP>:8080/dashboard.html`：

![仪表盘](screenshots/dashboard.png)

两个独立进程通过 MySQL 解耦：SystemMonitor 负责采集写入，http-server 负责查询展示。

## 功能特性

- **系统数据采集**：读取 `/proc/stat` 和 `/proc/meminfo`，CPU 两次采样差分计算使用率
- **生产者-消费者模型**：Producer 采集 → RingBuffer 缓冲 → Consumer 批量写入
- **RingBuffer 环形缓冲区**：容量 64，pthread_mutex + pthread_cond 线程安全，支持 shutdown 优雅退出
- **MySQL 批量存储**：每 10 条拼接一条多行 INSERT，减少磁盘 IO 和网络往返
- **信号处理**：SIGINT/SIGTERM/SIGQUIT/SIGHUP 优雅退出，退出前刷剩余数据
- **Web 仪表盘**：epoll 驱动 HTTP 服务器 + Chart.js 折线图，每 2 秒刷新

## 技术要点

| 技术 | 说明 |
|------|------|
| `/proc` 文件系统 | 解析 `/proc/stat`（CPU jiffies）和 `/proc/meminfo`（MemTotal/MemAvailable） |
| pthread 多线程 | Producer + Consumer 双线程，通过 RingBuffer 解耦 |
| 互斥锁与条件变量 | pthread_mutex_t 保护临界区，pthread_cond_t 实现阻塞等待而非忙轮询 |
| 环形缓冲区 | 固定大小数组 + head/tail/count，容量 64，2 的次方 |
| 信号处理 | sig_atomic_t 标志位 + ring_shutdown 广播唤醒 + storage_flush 刷盘 |
| MySQL C API | mysql_real_connect / mysql_query，多行 INSERT 批量写入，JSON API 查询 |
| epoll I/O 多路复用 | 非阻塞 socket + epoll_create1/epoll_wait + Keep-Alive 超时管理 |
| HTTP 协议 | 从零解析请求行、构造响应头、MIME 类型映射、路径安全 |
| Chart.js | 前端折线图，fetch API 每 2 秒轮询 JSON 数据 |

## 开发环境

| 项 | 说明 |
|------|------|
| 操作系统 | Ubuntu 虚拟机（Windows 宿主机） |
| 编译器 | GCC |
| 数据库 | MySQL（本地，用户 lzx） |
| 浏览器 | Windows 本机 Chrome，访问 VM IP:8080 |
| Shell | fish |

依赖：

- libmysqlclient（`apt install libmysqlclient-dev`）
- libpthread（系统自带）

## 项目结构

```
SystemMonitor/
├── main.c              # 入口：MySQL → RingBuffer → 线程 → 信号 → 清理
├── monitor.h / monitor.c   # 数据采集模块
├── ringbuffer.h / ringbuffer.c  # 环形缓冲区（线程安全）
├── producer.h / producer.c     # 生产者线程
├── consumer.h / consumer.c     # 消费者线程
├── storage.h / storage.c       # MySQL 批量存储
├── config.h               # 配置宏
├── monitor.conf           # 配置文件（示例）
├── README.md
├── CLAUDE.md
└── Linux系统监控项目实现流程说明.md

http-server/
├── main.c              # epoll 主循环 + Keep-Alive 管理
├── server.c / server.h     # socket/bind/listen/accept
├── http.c / http.h         # HTTP 请求解析、响应构造、路由分发
├── file.c / file.h         # 静态文件服务、MIME 检测
├── api.c / api.h           # /api/data JSON 接口
└── www/
    ├── index.html          # 首页
    └── dashboard.html      # 监控仪表盘
```

## 编译运行

### SystemMonitor

```bash
cd SystemMonitor
gcc *.c -o main -lpthread -lmysqlclient -Wall
./main
```

### http-server

```bash
cd http-server
gcc -o http_server main.c server.c http.c file.c api.c -lmysqlclient -Wall
./http_server
```

浏览器打开 `http://<主机IP>:8080/dashboard.html` 查看仪表盘
（本机可用 `http://127.0.0.1:8080/dashboard.html`）。

## 数据库

```sql
CREATE DATABASE system_monitor;
USE system_monitor;

CREATE TABLE monitor_data (
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    cpu_usage  FLOAT       NOT NULL COMMENT 'CPU使用率(%)',
    mem_usage  FLOAT       NOT NULL COMMENT '内存使用率(%)',
    timestamp  BIGINT      NOT NULL COMMENT '采集时间戳',
    created_at DATETIME    DEFAULT CURRENT_TIMESTAMP
);
```

## API

| 接口 | 方法 | 说明 |
|------|------|------|
| `/` | GET | 首页 |
| `/dashboard.html` | GET | 监控仪表盘 |
| `/api/data` | GET | 返回最近 60 条监控数据（JSON） |

JSON 格式：

```json
[
  {"cpu": 1.50, "mem": 47.20, "time": 1786369647},
  {"cpu": 2.00, "mem": 47.50, "time": 1786369648}
]
```

## 许可证

MIT
