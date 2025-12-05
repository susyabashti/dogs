#include "civetweb.h"
#include "string.h"
#include "cJSON.h"
#include "stdlib.h"
#include "time.h"
#include "services/db_health/db_health.h"

#define SERVICE_NAME "dogs-api"
#define VERSION "0.1.0"

int health_handler(struct mg_connection *conn, void *ignored)
{
  (void)ignored;

  time_t now = time(NULL);
  DBStatus db_status = db_health_status();

  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "app", SERVICE_NAME);
  cJSON_AddStringToObject(json, "version", VERSION);
  cJSON_AddBoolToObject(json, "db_status", db_status == DB_STATUS_UP);
  cJSON_AddNumberToObject(json, "timestamp", (double)now);

  char *str = cJSON_PrintUnformatted(json);
  unsigned long len = (unsigned long)strlen(str);
  mg_send_http_ok(conn, "application/json", len);
  mg_write(conn, str, len);
  cJSON_free(str);
  cJSON_Delete(json);
  return 200;
}
