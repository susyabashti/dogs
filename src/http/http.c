#include "civetweb.h"
#include "logging/logging.h"

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
    log_msg(LOG_ERROR, "[HTTP] Failed to start CivetWeb server.");
    return NULL;
  }

  return ctx;
}

void http_server_stop(void)
{
  if (!ctx)
  {
    log_msg(LOG_ERROR, "[HTTP] Expected a running server to stop, but got null pointer.");
    return;
  }

  mg_stop(ctx);
  mg_exit_library();
  log_msg(LOG_INFO, "[HTTP] Server stopped");
}