#include "civetweb.h"

static struct mg_context *ctx = NULL;

struct mg_context *http_server_start(const char *port)
{
  mg_init_library(0);

  const char *options[] = {
      "listening_ports",
      port,
      NULL,
  };

  ctx = mg_start(NULL, NULL, options);

  if (ctx == NULL)
  {
    fprintf(stderr, "[http] Failed to start Civetweb server.\n");
    return NULL;
  }

  return ctx;
}

void http_server_stop(void)
{
  if (!ctx)
  {
    fprintf(stderr, "[http] ctx is NULL, cannot stop server.");
    return;
  }

  mg_stop(ctx);
  mg_exit_library();
  printf("Server stopped.\n");
}