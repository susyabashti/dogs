#pragma once

#include <stdbool.h>

// Initialize the DB health checker thread
void db_health_init(void);

// Get the current cached DB status ("up" / "down")
const char *db_health_status(void);
