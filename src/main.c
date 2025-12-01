#include "stdio.h"
#include "civetweb.h"
#include "http/http.h"
#include "http/router.h"
#include "db/db.h"
#include "string.h"
#include "db_health/db_health.h"
#include "unistd.h"

#define PORT "8080"

static int keep_running = 1;

void sigint_handler(int sig)
{
  (void)sig;
  keep_running = 0;
}

int main(void)
{
  if (db_pool_init(5) != 0)
  {
    fprintf(stderr, "Failed to initialize DB pool\n");
    return 1;
  }

  db_health_init();

  struct mg_context *ctx = http_server_start(PORT);

  if (!ctx)
  {
    fprintf(stderr, "Failed to start Civetweb server.\n");
    return 1;
  }

  register_routes(ctx);
  printf("Server started on http://127.0.0.1:%s. Press Enter to stop...\n", PORT);

  while (keep_running)
  {
    sleep(1);
  }

  http_server_stop();
  db_pool_destroy();

  return 0;
}
