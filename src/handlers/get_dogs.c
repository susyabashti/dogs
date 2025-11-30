#include "civetweb.h"
#include "string.h"

int get_dogs_handler(struct mg_connection *conn, void *ignored)
{
  (void)ignored;

  const char *msg = "GOT GET REQUEST";
  const struct mg_request_info *request = mg_get_request_info(conn);

  if (strcmp(request->request_method, "POST") == 0)
  {
    msg = "GOT POST REQUEST";
  }

  unsigned long len = (unsigned long)strlen(msg);
  mg_send_http_ok(conn, "text/plain", len);
  mg_write(conn, msg, len);

  return 200;
}
