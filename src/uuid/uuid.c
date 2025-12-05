#include "ctype.h"
#include "string.h"
#include "stdbool.h"

bool is_valid_uuid(const char *uuid)
{
  if (!uuid)
    return false;
  if (strlen(uuid) != 36)
    return false;

  for (int i = 0; i < 36; i++)
  {
    char c = uuid[i];
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      if (c != '-')
        return false;
    }
    else
    {
      if (!isxdigit(c))
        return false;
    }
  }
  return true;
}
