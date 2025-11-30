#include "router.h"
#include "handlers/handlers.h"
#include "civetweb.h"
#include <string.h>
#include <stdio.h>

struct route routes[] = {
    {"/api/health", health_handler},
    {"/api/dogs", get_dogs_handler},
    {NULL, NULL}, // sentinel
};

void register_routes(struct mg_context *ctx)
{
    if (!ctx)
    {
        fprintf(stderr, "[router] Error: ctx is NULL, cannot register routes\n");
        return;
    }

    for (int i = 0; routes[i].path != NULL; i++)
    {
        mg_set_request_handler(ctx, routes[i].path, routes[i].handler, NULL);
        printf("[router] Registered route: %s\n", routes[i].path);
    }
}
