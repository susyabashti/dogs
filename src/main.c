#include "stdio.h"
#include "unistd.h"
#include "civetweb.h"
#include "string.h"
#include "time.h"
#include "db/db.h"
#include "http/http.h"
#include "http/router.h"
#include "services/db_health/db_health.h"
#include "services/dog_service/dog_service.h"
#include "logging/logging.h"

#define PORT "8080"
#define DB_POOL_SIZE 32

static int keep_running = 1;

void sigint_handler(int sig)
{
  (void)sig;
  keep_running = 0;
}

int main(void)
{
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  register_dog_service_prepared_statements();

  if (db_pool_init(DB_POOL_SIZE) != 0)
  {
    log_msg(LOG_ERROR, "Failed to initialize DB pool");
    return 1;
  }

  db_health_init();

  struct mg_context *ctx = http_server_start(PORT);

  if (!ctx)
  {
    log_msg(LOG_ERROR, "Failed to start Civetweb server.");
    return 1;
  }

  register_routes(ctx);

  clock_gettime(CLOCK_MONOTONIC, &end);

  long seconds = end.tv_sec - start.tv_sec;
  long nanoseconds = end.tv_nsec - start.tv_nsec;
  long microseconds = seconds * 1000000 + nanoseconds / 1000;

  log_msg(LOG_INFO, "Server started on http://127.0.0.1:%s (startup time: %ld µs)", PORT, microseconds);

  while (keep_running)
  {
    sleep(1);
  }

  http_server_stop();
  db_pool_destroy();

  return 0;
}
