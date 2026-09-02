# 交接文档：Web 展示集成

## 项目概况

两个独立项目，通过 MySQL 解耦：

| 项目 | 路径 | 状态 | 职责 |
|------|------|------|------|
| SystemMonitor | `/home/lzx/SystemMonitor` | 基本完成 | 采集系统数据 → 写入 MySQL |
| http-server | `/home/lzx/http-server` | 基础完成 | 静态 HTTP 服务器，epoll 驱动 |

## 集成目标

http-server 新增 `GET /api/data` 接口，返回 JSON 格式的监控数据。
浏览器打开仪表盘页面，JS 定时请求 API，用 Chart.js 画 CPU / 内存折线图。

架构：

```
SystemMonitor                    http-server
  ./main                          ./http_server
    │                                │
    ├─ Producer                     ├─ /index.html（仪表盘页面）
    ├─ RingBuffer                   ├─ /api/data（新增，查 MySQL 返 JSON）
    ├─ Consumer → MySQL             └─ 其他静态文件服务（不变）
    └─ 不改动 ✓                    └─ 需要修改 ✗
                  │
                  ▼
              MySQL (system_monitor.monitor_data)
```

---

## 二、SystemMonitor 当前状态（已完成）

### 功能

- 每 100ms 采集一次 CPU 和内存使用率
- Producer → RingBuffer(容量64) → Consumer 线程模型
- mutex + cond 线程安全
- Consumer 每 10 条一批写入 MySQL
- SIGINT/SIGTERM/SIGQUIT/SIGHUP 优雅退出，shutdown 时刷剩余数据
- config.h 宏定义配置文件

### 编译

```
gcc *.c -o main -lpthread -lmysqlclient -Wall
./main
```

### 文件清单

```
main.c          — 入口：MySQL → RingBuffer → 线程 → 信号 → 清理
monitor.h/c     — 数据采集：get_cpu_usage(), get_mem_usage(), collect_monitor_data()
ringbuffer.h/c  — 环形缓冲区：ring_push/pop，mutex+cond+shutdown
producer.h/c    — 生产者线程：采集 → ring_push
consumer.h/c    — 消费者线程：ring_pop → 打印 + storage_save
storage.h/c     — MySQL 存储：storage_init/save/flush/close，10条批量 INSERT
config.h        — 配置宏：MySQL连接信息、采集间隔
```

### 数据库

```
库：system_monitor    用户：lzx    密码：123456

monitor_data 表：
  id         BIGINT AUTO_INCREMENT PRIMARY KEY
  cpu_usage  FLOAT
  mem_usage  FLOAT
  timestamp  BIGINT       — 采集时 Unix 时间戳
  created_at DATETIME     — 写入时 MySQL 自动生成
```

---

## 三、http-server 当前状态

### 功能

- 静态文件服务（www/ 目录映射到 URL）
- epoll 事件驱动，非阻塞 I/O
- Keep-Alive 长连接，5 秒超时
- MIME 类型自动检测
- 目录浏览
- HEAD 方法支持
- 路径遍历防护

### 架构

```
main.c    — 主循环：epoll_wait + Keep-Alive 超时管理
server.c  — 网络层：socket/bind/listen/accept
http.c    — 协议层：解析请求行、构造响应头、路由分发
file.c    — 文件层：MIME检测、路径安全、读取文件内容
www/      — 静态文件根目录
```

### 编译

```
gcc -o http_server main.c server.c http.c file.c -Wall
./http_server
```

监听 8080 端口。

### 请求处理流程

```
handle_request(client_fd, buf)
  │
  ├─ parse_request_line(buf, &method, &url, &version)
  │
  ├─ GET  → serve_file(client_fd, url)     — 静态文件
  ├─ HEAD → serve_file_head(client_fd, url) — 只返回头
  └─ 其他 → send_error 405
```

---

## 四、集成改动清单（在 http-server 项目中进行）

### 4.1 新建 api.h

```c
#ifndef API_H
#define API_H

// 查询最近 limit 条监控数据，返回 JSON 字符串
// caller 负责 free 返回值
char *api_query_data(int limit);

// 处理 /api/data 请求
void handle_api(int client_fd);

#endif
```

### 4.2 新建 api.c

实现：

1. `api_query_data(int limit)`：
   - `mysql_init` → `mysql_real_connect(localhost, lzx, 123456, system_monitor)`
   - 执行 `SELECT cpu_usage, mem_usage, timestamp FROM monitor_data ORDER BY id DESC LIMIT N`
   - 遍历 `mysql_store_result` 结果集
   - 拼接 JSON：`[{"cpu":1.5,"mem":48.0,"time":123},...]`
   - `mysql_close`

2. `handle_api(int client_fd)`：
   - 调 `api_query_data(60)` 拿 JSON 字符串
   - 调 `send_header(client_fd, 200, "OK", "application/json; charset=utf-8", strlen(json))`
   - `send(client_fd, json, strlen(json), 0)`
   - `free(json)`

注意 JSON 拼接时要处理内存分配，建议先算长度再 malloc，或用 realloc 逐步扩容。

### 4.3 修改 http.c

在 `handle_request` 函数中，`serve_file` 之前加 API 路由：

```c
if (strncmp(url, "/api/data", 9) == 0)
{
    handle_api(client_fd);
    return;
}
serve_file(client_fd, url);
```

### 4.4 修改 www/index.html

仪表盘页面，包含：

- Chart.js CDN 引入
- 两个 `<canvas>`：CPU 折线图 + 内存折线图
- JS 逻辑：
  - `fetch('/api/data')` 获取 JSON
  - 解析数据，逆序（API 返回最新在前，图表需要时间从左到右递增）
  - 更新 Chart.js 图表数据
  - `setInterval(fetchAndRender, 2000)` 每 2 秒刷新

### 4.5 修改编译命令 / Makefile

加上 MySQL 库：

```
gcc -o http_server main.c server.c http.c file.c api.c -lmysqlclient -Wall
```

或修改 Makefile 的 OBJS 加上 api.o，LDFLAGS 加上 `-lmysqlclient`。

---

## 五、验证步骤

1. 启动 SystemMonitor：`cd /home/lzx/SystemMonitor && ./main &`
2. 启动 http-server：`cd /home/lzx/http-server && ./http_server &`
3. 浏览器打开 `http://localhost:8080/index.html`，看仪表盘
4. 终端测试 API：`curl http://localhost:8080/api/data`，应返回 JSON
5. Ctrl+C 停止 http-server，kill 停止 SystemMonitor

---

## 六、后续可优化

- API 加查询参数（`/api/data?limit=100`）
- 连接池复用（不用每次请求都 mysql_init/close）
- 支持 CORS（前后端分离部署时）
- WebSocket 实时推送（替代轮询）
