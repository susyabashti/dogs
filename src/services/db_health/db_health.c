#include "pthread.h"
#include "string.h"
#include "unistd.h"
#include "db_health.h"
#include "db/db.h"

#define UPDATE_INTERVAL_SECONDS 30

static DBStatus cached_status = DB_STATUS_DOWN;
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

static void update_db_status(void)
{
    PGconn *conn = db_pool_acquire();
    if (conn && PQstatus(conn) == CONNECTION_OK)
    {
        pthread_mutex_lock(&status_mutex);
        cached_status = DB_STATUS_UP;
        pthread_mutex_unlock(&status_mutex);
    }
    else
    {
        pthread_mutex_lock(&status_mutex);
        cached_status = DB_STATUS_DOWN;
        pthread_mutex_unlock(&status_mutex);
    }
    if (conn)
        db_pool_release(conn);
}

static void *db_health_thread(void *arg)
{
    (void)arg;
    while (1)
    {
        update_db_status();
        sleep(UPDATE_INTERVAL_SECONDS);
    }
    return NULL;
}

void db_health_init(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, db_health_thread, NULL);
    pthread_detach(tid);
}

DBStatus db_health_status(void)
{
    pthread_mutex_lock(&status_mutex);
    int status = cached_status;
    pthread_mutex_unlock(&status_mutex);
    return status;
}
