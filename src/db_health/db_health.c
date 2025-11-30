#include "db_health.h"
#include "db/db.h" // your pool functions
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define STATUS_UP "up"
#define STATUS_DOWN "down"
#define UPDATE_INTERVAL 15 // seconds

static char cached_status[16] = STATUS_DOWN;
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

static void update_db_status(void)
{
    PGconn *conn = db_pool_acquire();
    if (conn && PQstatus(conn) == CONNECTION_OK)
    {
        pthread_mutex_lock(&status_mutex);
        strncpy(cached_status, STATUS_UP, sizeof(cached_status));
        pthread_mutex_unlock(&status_mutex);
    }
    else
    {
        pthread_mutex_lock(&status_mutex);
        strncpy(cached_status, STATUS_DOWN, sizeof(cached_status));
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
        sleep(UPDATE_INTERVAL);
    }
    return NULL;
}

void db_health_init(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, db_health_thread, NULL);
    pthread_detach(tid);
}

const char *db_health_status(void)
{
    pthread_mutex_lock(&status_mutex);
    const char *status = cached_status;
    pthread_mutex_unlock(&status_mutex);
    return status;
}
