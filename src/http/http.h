#pragma once

#include "civetweb.h"

struct mg_context *http_server_start(const char *port);

void http_server_stop(void);