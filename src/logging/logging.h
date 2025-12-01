#pragma once

#include <stdio.h>
#include <stdarg.h>

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void log_msg(log_level_t level, const char *fmt, ...);
