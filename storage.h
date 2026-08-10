#ifndef STORAGE_H
#define STORAGE_H

#include "monitor.h"
#include <mysql/mysql.h>

#define BATCH_SIZE 10

// 初始化 MySQL 连接
MYSQL *storage_init(const char *host,const char *user,const char *passwd,const char *db);

// 将一条数据放入攒批缓冲区，满 BATCH_SIZE 条自动提交
int storage_save(MYSQL *conn,MonitorData *data);

// 将不足一批的剩余数据刷入数据库
void storage_flush(MYSQL *conn);

// 关闭 MySQL 连接
void storage_close(MYSQL *conn);

#endif
