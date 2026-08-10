#include "storage.h"

#include <stdio.h>
#include <string.h>

MYSQL *storage_init(const char *host,const char *user,const char *passwd,const char *db)
{
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL)
    {
        fprintf(stderr,"mysql_init() failed\n");
        return NULL;
    }

    if (mysql_real_connect(conn,host,user,passwd,db,0,NULL,0) == NULL)
    {
        fprintf(stderr,"mysql_real_connect() failed: %s\n",mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    printf("[Storage] MySQL connected: %s@%s/%s\n",user,host,db);
    return conn;
}

int storage_save(MYSQL *conn,MonitorData *data)
{
    char sql[256];

    // 拼接 INSERT 语句
    snprintf(sql,sizeof(sql),
             "INSERT INTO monitor_data(cpu_usage,mem_usage,timestamp) "
             "VALUES(%.2f,%.2f,%ld)",
             data->cpu_usage,
             data->mem_usage,
             data->timestamp);

    if (mysql_query(conn,sql) != 0)
    {
        fprintf(stderr,"[Storage] INSERT failed: %s\n",mysql_error(conn));
        return -1;
    }

    return 0;
}

void storage_close(MYSQL *conn)
{
    if (conn != NULL)
    {
        mysql_close(conn);
        printf("[Storage] MySQL disconnected\n");
    }
}
