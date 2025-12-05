#pragma once

#include "civetweb.h"

int health_handler(struct mg_connection *conn, void *ignored);
int dogs_dispatcher_handler(struct mg_connection *conn, void *ignored);