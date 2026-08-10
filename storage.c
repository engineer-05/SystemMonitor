#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 批量写入：字符串拼接多行 INSERT，不用事务、不用预处理。
 *
 *   日常：storage_save 只把数据放入攒批缓冲区，满 BATCH_SIZE 条后
 *        拼接一条多行 INSERT SQL 执行。
 *
 *   退出：storage_flush 将不足一批的剩余数据写库。
 */

// 批量缓冲区（模块内部使用）
static struct
{
    float      cpu_data[BATCH_SIZE];
    float      mem_data[BATCH_SIZE];
    long long  ts_data[BATCH_SIZE];
    int        count;
} g_batch = { .count = 0 };


MYSQL *storage_init(const char *host,
                    const char *user,
                    const char *passwd,
                    const char *db)
{
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL)
    {
        fprintf(stderr,"mysql_init() failed\n");
        return NULL;
    }

    if (mysql_real_connect(conn,host,user,passwd,db,0,NULL,0) == NULL)
    {
        fprintf(stderr,"mysql_real_connect() failed: %s\n",
                mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    printf("[Storage] MySQL connected: %s@%s/%s (batch=%d)\n",
           user,host,db,BATCH_SIZE);
    return conn;
}

// ---------------------------------------------------------------------------
// 将 N 条数据拼成多行 INSERT 并执行
// ---------------------------------------------------------------------------
static int insert_rows(MYSQL *conn, int n)
{
    // 估算 SQL 长度：前缀 + N × "(1.23,45.67,1234567890)," + 结尾
    size_t size = 256 + n * 64;
    char  *sql  = malloc(size);
    if (sql == NULL) return -1;

    int pos = snprintf(sql, size,
                       "INSERT INTO monitor_data(cpu_usage,mem_usage,timestamp) VALUES ");
    for (int i = 0; i < n; i++)
    {
        pos += snprintf(sql + pos, size - pos,
                        "%s(%.2f,%.2f,%lld)",
                        (i > 0 ? "," : ""),
                        g_batch.cpu_data[i],
                        g_batch.mem_data[i],
                        g_batch.ts_data[i]);
    }

    int ret = mysql_query(conn, sql);
    if (ret != 0)
        fprintf(stderr,"[Storage] INSERT failed: %s\n", mysql_error(conn));

    free(sql);
    return ret;
}

// ---------------------------------------------------------------------------
// 攒批：满 BATCH_SIZE 条自动提交
// ---------------------------------------------------------------------------
int storage_save(MYSQL *conn, MonitorData *data)
{
    int i = g_batch.count;

    g_batch.cpu_data[i] = data->cpu_usage;
    g_batch.mem_data[i] = data->mem_usage;
    g_batch.ts_data[i]  = data->timestamp;
    g_batch.count++;

    if (g_batch.count < BATCH_SIZE)
        return 0;

    // 满一批，写入，写入失败直接丢弃，后续可以改为写入文件兜底
    if (insert_rows(conn, BATCH_SIZE) != 0)
    {
        g_batch.count = 0;
        return -1;
    }

    printf("[Storage] batch commit: %d rows\n", BATCH_SIZE);
    fflush(stdout);

    g_batch.count = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// 退出前刷剩余数据
// ---------------------------------------------------------------------------
void storage_flush(MYSQL *conn)
{
    if (g_batch.count == 0) return;

    if (insert_rows(conn, g_batch.count) != 0)
    {
        g_batch.count = 0;
        return;
    }

    printf("[Storage] flush: %d rows\n", g_batch.count);
    fflush(stdout);

    g_batch.count = 0;
}

// ---------------------------------------------------------------------------
// 关闭连接
// ---------------------------------------------------------------------------
void storage_close(MYSQL *conn)
{
    if (conn != NULL)
    {
        mysql_close(conn);
        printf("[Storage] MySQL disconnected\n");
    }
}
