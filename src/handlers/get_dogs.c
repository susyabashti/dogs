#include "civetweb.h"
#include "string.h"
#include "db/db.h"
#include "cJSON.h"
#include "stdlib.h"
#include "logging/logging.h"

int get_dogs_handler(struct mg_connection *conn, void *ignored)
{
  (void)ignored;
  const struct mg_request_info *request = mg_get_request_info(conn);

  if (strcmp(request->request_method, "GET") != 0)
  {
    mg_send_http_error(conn, 404, "Page not found");
    return 400;
  }

  const char *uri = request->local_uri;
  const char *prefix = "/api/dogs/";
  const char *dog_id = NULL;

  if (strncmp(uri, prefix, strlen(prefix)) == 0)
  {
    dog_id = uri + strlen(prefix);
  }

  if (!dog_id || strlen(dog_id) == 0)
  {
    mg_send_http_error(conn, 400, "Bad request");
    return 400;
  }

  PGconn *db_conn = db_pool_acquire();
  if (!db_conn)
  {
    mg_send_http_error(conn, 500, "DB connection couldn't established.");
    return 500;
  }

  const char *paramValues[1];
  paramValues[0] = dog_id;

  PGresult *result = PQexecParams(db_conn, "SELECT * FROM dogs WHERE id = $1", 1, NULL, paramValues, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    log_msg(LOG_ERROR, "DB query failed for dog_id=%s: %s", dog_id, PQerrorMessage(db_conn));
    PQclear(result);
    db_pool_release(db_conn);
    mg_send_http_error(conn, 500, "Query failed");
    return 500;
  }

  int rows = PQntuples(result);
  if (rows == 0)
  {
    PQclear(result);
    db_pool_release(db_conn);
    mg_send_http_error(conn, 404, "Dog not found");
    return 404;
  }

  // Convert row to JSON
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "id", PQgetvalue(result, 0, 0));
  cJSON_AddStringToObject(json, "breed", PQgetvalue(result, 0, 1));
  cJSON_AddStringToObject(json, "country_origin", PQgetvalue(result, 0, 2));
  cJSON_AddStringToObject(json, "fur_color", PQgetvalue(result, 0, 3));
  cJSON_AddStringToObject(json, "eye_colors", PQgetvalue(result, 0, 4));
  cJSON_AddNumberToObject(json, "min_height", atoi(PQgetvalue(result, 0, 5)));
  cJSON_AddNumberToObject(json, "max_height", atoi(PQgetvalue(result, 0, 6)));
  cJSON_AddNumberToObject(json, "min_longevity", atoi(PQgetvalue(result, 0, 7)));
  cJSON_AddNumberToObject(json, "max_longevity", atoi(PQgetvalue(result, 0, 8)));
  cJSON_AddStringToObject(json, "character_traits", PQgetvalue(result, 0, 9));
  cJSON_AddStringToObject(json, "common_health_problems", PQgetvalue(result, 0, 10));

  char *str = cJSON_PrintUnformatted(json);

  mg_send_http_ok(conn, "application/json", strlen(str));
  mg_write(conn, str, strlen(str));

  // Cleanup
  cJSON_free(str);
  cJSON_Delete(json);
  PQclear(result);
  db_pool_release(db_conn);

  return 200;
}
