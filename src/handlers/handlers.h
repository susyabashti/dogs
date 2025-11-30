#pragma once

#include "civetweb.h"

int health_handler(struct mg_connection *conn, void *ignored);
int get_dogs_handler(struct mg_connection *conn, void *ignored);
