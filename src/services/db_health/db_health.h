#pragma once

#include <stdbool.h>

typedef enum
{
  DB_STATUS_DOWN = 0,
  DB_STATUS_UP = 1
} DBStatus;

// Initialize the DB health checker thread
void db_health_init(void);

// Get the current cached DB status ("up" / "down")
DBStatus db_health_status(void);
