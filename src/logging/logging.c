#include "logging.h"

void log_msg(log_level_t level, const char *fmt, ...)
{
  const char *level_str = "";
  switch (level)
  {
  case LOG_INFO:
    level_str = "INFO";
    break;
  case LOG_WARN:
    level_str = "WARN";
    break;
  case LOG_ERROR:
    level_str = "ERROR";
    break;
  }

  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[%s] ", level_str);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}
