#include "db.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "logging/logging.h"

typedef struct
{
  char *name;
  char *sql;
} PreparedStmt;

static PGconn **pool = NULL;
static size_t pool_size = 0;
static int *available = NULL;
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

static PreparedStmt *prepared_stmts = NULL;
static size_t num_prepared_stmts = 0;

int db_register_prepared(const char *name, const char *sql)
{
  prepared_stmts = realloc(prepared_stmts, sizeof(PreparedStmt) * (num_prepared_stmts + 1));
  if (!prepared_stmts)
    return -1;

  prepared_stmts[num_prepared_stmts].name = strdup(name);
  prepared_stmts[num_prepared_stmts].sql = strdup(sql);
  num_prepared_stmts++;
  return 0;
}

static int db_prepare_statements(PGconn *conn)
{
  for (size_t i = 0; i < num_prepared_stmts; i++)
  {
    PGresult *res = PQprepare(conn,
                              prepared_stmts[i].name,
                              prepared_stmts[i].sql,
                              0, // number of params = 0 for generic
                              NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      log_msg(LOG_ERROR, "[DB] Failed to prepare statement %s: %s",
              prepared_stmts[i].name, PQerrorMessage(conn));
      PQclear(res);
      return -1;
    }
    PQclear(res);
  }
  return 0;
}

static PGconn *db_new_conn_internal(void)
{
  const char *host = getenv("DB_HOST");
  const char *dbName = getenv("DB_NAME");
  const char *user = getenv("DB_USER");
  const char *pass = getenv("DB_PASSWORD");
  const char *port = getenv("DB_PORT");

  if (!dbName || !user || !pass)
  {
    log_msg(LOG_ERROR, "[DB] Missing environment variables.");
    return NULL;
  }
  if (!host)
    host = "localhost";
  if (!port)
    port = "5432";

  char conninfo[256];
  snprintf(conninfo, sizeof(conninfo),
           "host=%s dbname=%s user=%s password=%s port=%s",
           host, dbName, user, pass, port);

  PGconn *conn = PQconnectdb(conninfo);
  if (PQstatus(conn) != CONNECTION_OK)
  {
    log_msg(LOG_ERROR, "[DB] Connection failed: %s", PQerrorMessage(conn));
    PQfinish(conn);
    return NULL;
  }

  // Prepare all registered statements
  if (db_prepare_statements(conn) != 0)
  {
    PQfinish(conn);
    return NULL;
  }

  return conn;
}

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

void db_pool_destroy(void)
{
  for (size_t i = 0; i < num_prepared_stmts; i++)
  {
    free(prepared_stmts[i].name);
    free(prepared_stmts[i].sql);
  }
  free(prepared_stmts);
  prepared_stmts = NULL;
  num_prepared_stmts = 0;

  for (size_t i = 0; i < pool_size; i++)
  {
    if (!pool[i])
      continue;

    PQfinish(pool[i]);
  }

  free(pool);
  free(available);
  pool = NULL;
  available = NULL;
  pool_size = 0;
}