#include "db.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static PGconn **pool = NULL;
static size_t pool_size = 0;
static int *available = NULL;
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

// Helper: create a new PG connection
static PGconn *db_new_conn_internal(void)
{
  const char *host = getenv("DB_HOST");
  const char *dbName = getenv("DB_NAME");
  const char *user = getenv("DB_USER");
  const char *pass = getenv("DB_PASSWORD");
  const char *port = getenv("DB_PORT");

  if (!dbName || !user || !pass)
  {
    fprintf(stderr, "[DB] Missing environment variables\n");
    return NULL;
  }
  if (!port)
    port = "5432";

  char conninfo[256];
  size_t n = snprintf(conninfo, sizeof(conninfo),
                      "host=%s dbname=%s user=%s password=%s port=%s",
                      host, dbName, user, pass, port);
  if (n >= sizeof(conninfo))
  {
    fprintf(stderr, "[DB] Connection string too long\n");
    return NULL;
  }

  PGconn *conn = PQconnectdb(conninfo);
  if (PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "[DB] Connection failed: %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return NULL;
  }
  return conn;
}

// Initialize pool
int db_pool_init(size_t size)
{
  pool = calloc(size, sizeof(PGconn *));
  available = calloc(size, sizeof(int));
  if (!pool || !available)
    return -1;

  pool_size = size;
  for (size_t i = 0; i < size; i++)
  {
    pool[i] = db_new_conn_internal();
    if (!pool[i])
      return -1;
    available[i] = 1;
  }
  return 0;
}

// Acquire a connection (blocking)
PGconn *db_pool_acquire(void)
{
  PGconn *conn = NULL;
  pthread_mutex_lock(&pool_mutex);
  while (!conn)
  {
    for (size_t i = 0; i < pool_size; i++)
    {
      if (available[i])
      {
        available[i] = 0;
        conn = pool[i];
        break;
      }
    }
    if (!conn)
    {
      pthread_cond_wait(&pool_cond, &pool_mutex);
    }
  }
  pthread_mutex_unlock(&pool_mutex);
  return conn;
}

// Release a connection back to the pool
void db_pool_release(PGconn *conn)
{
  pthread_mutex_lock(&pool_mutex);
  for (size_t i = 0; i < pool_size; i++)
  {
    if (pool[i] == conn)
    {
      available[i] = 1;
      pthread_cond_signal(&pool_cond);
      break;
    }
  }
  pthread_mutex_unlock(&pool_mutex);
}

// Destroy pool
void db_pool_destroy(void)
{
  if (!pool)
    return;
  for (size_t i = 0; i < pool_size; i++)
  {
    if (pool[i])
      PQfinish(pool[i]);
  }
  free(pool);
  free(available);
  pool = NULL;
  available = NULL;
  pool_size = 0;
}
