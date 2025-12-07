#include "time.h"
#include "stdlib.h"

time_t portable_timegm(struct tm *tm)
{
  char *old_tz = getenv("TZ");

  setenv("TZ", "UTC", 1);
  tzset();

  time_t t = mktime(tm);

  if (old_tz)
    setenv("TZ", old_tz, 1);
  else
    unsetenv("TZ");

  tzset();
  return t;
}
