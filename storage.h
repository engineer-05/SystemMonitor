#ifndef STORAGE_H
#define STORAGE_H

#include "monitor.h"
#include <mysql/mysql.h>

// 初始化 MySQL 连接
MYSQL *storage_init(const char *host,const char *user,const char *passwd,const char *db);

// 将一条监控数据写入数据库
int storage_save(MYSQL *conn,MonitorData *data);

// 关闭 MySQL 连接
void storage_close(MYSQL *conn);

#endif
