#include "router.h"
#include "handlers/handlers.h"
#include "civetweb.h"
#include "string.h"
#include "stdio.h"
#include "logging/logging.h"

struct route routes[] = {
    {"/api/health", health_handler},
    {"/api/dogs", dogs_dispatcher_handler},
    {"/api/dogs/*", dogs_dispatcher_handler},
    {NULL, NULL}, // sentinel
};

void register_routes(struct mg_context *ctx)
{
    if (!ctx)
    {
        log_msg(LOG_ERROR, "[HTTP] Expected context of http service, but got null. cannot register routes.");
        return;
    }

    for (int i = 0; routes[i].path != NULL; i++)
    {
        mg_set_request_handler(ctx, routes[i].path, routes[i].handler, NULL);
        log_msg(LOG_INFO, "[HTTP] Registered route: %s", routes[i].path);
    }
}
