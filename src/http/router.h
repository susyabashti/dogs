#pragma once

#include "civetweb.h"

typedef int (*route_handler)(struct mg_connection *, void *);

struct route
{
  const char *path;
  route_handler handler;
};

extern struct route routes[];

void register_routes(struct mg_context *ctx);