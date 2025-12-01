#include "civetweb.h"
#include "string.h"
#include "cJSON.h"
#include "stdlib.h"
#include "time.h"
#include "db_health/db_health.h"

int health_handler(struct mg_connection *conn, void *ignored)
{
  (void)ignored;

  time_t now = time(NULL);
  const char *db_status = db_health_status();

  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "app", "dogs_api");
  cJSON_AddStringToObject(json, "version", "0.1.0");
  cJSON_AddStringToObject(json, "db_status", db_status);
  cJSON_AddNumberToObject(json, "timestamp", (double)now);

  char *str = cJSON_PrintUnformatted(json);
  unsigned long len = (unsigned long)strlen(str);
  mg_send_http_ok(conn, "application/json", len);
  mg_write(conn, str, len);
  cJSON_free(str);
  cJSON_Delete(json);
  return 200;
}
