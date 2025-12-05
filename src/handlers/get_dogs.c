#include "civetweb.h"
#include "string.h"
#include "db/db.h"
#include "cJSON.h"
#include "stdlib.h"
#include "logging/logging.h"
#include "services/dog_service/dog_service.h"
#include "uuid/uuid.h"

#define DEFAULT_LIMIT 10
#define DEFAULT_LIMIT_STR "10"
#define MAX_PER_REQUEST 25
#define DEFAULT_CURSOR_STR "00000000-0000-0000-0000-000000000000"

int get_all_dogs_handler(struct mg_connection *conn, const struct mg_request_info *request)
{
  const char *query_string = request->query_string;
  size_t query_string_len;
  if (query_string)
  {
    query_string_len = strlen(query_string);
  }

  int limit = DEFAULT_LIMIT;
  char *next_cursor = DEFAULT_CURSOR_STR;

  if (query_string)
  {
    char limit_str[16];
    char next_cursor_str[37];
    limit_str[0] = '\0';
    next_cursor_str[0] = '\0';

    mg_get_var(query_string, query_string_len, "limit", limit_str, sizeof(limit_str));
    mg_get_var(query_string, query_string_len, "next_cursor", next_cursor_str, sizeof(next_cursor_str));

    if (strlen(next_cursor_str) > 0)
    {
      if (!is_valid_uuid(next_cursor_str))
      {
        mg_send_http_error(conn, 400, "Invalid next_cursor format.");
        return 400;
      }

      next_cursor = next_cursor_str;
    }

    if (strlen(limit_str) > 0)
    {
      limit = atoi(limit_str);

      if (limit <= 0)
        limit = DEFAULT_LIMIT;
      else if (limit > MAX_PER_REQUEST)
        limit = MAX_PER_REQUEST;
    }
  }

  PGconn *db_conn = db_pool_acquire();
  if (!db_conn)
  {
    mg_send_http_error(conn, 500, "DB connection couldn't established.");
    return 500;
  }
  Dog **dogs;
  GetDogsByQueryStatus status = get_dogs_by_query(db_conn, next_cursor, limit, &dogs);

  // Release db connection
  db_pool_release(db_conn);

  if (status == GET_DOGS_BY_QUERY_ERROR)
  {
    mg_send_http_error(conn, 500, "Error retrieving dogs.");
    return 500;
  }

  cJSON *json = cJSON_CreateObject();
  cJSON *array = cJSON_CreateArray();

  size_t count = 0;
  char next_cursor_result[37] = "";
  char *last_id = NULL;

  if (status != GET_DOGS_BY_QUERY_EMPTY)
  {
    for (int i = 0; dogs[i] != NULL; i++)
    {
      cJSON *dog_json = dog_to_json(dogs[i]);
      cJSON_AddItemToArray(array, dog_json);
      count++;

      last_id = strdup(dogs[i]->id);
      dog_free(dogs[i]); // <-- we free the dog item from memory
    }

    free(dogs); // <-- we free the dogs array from memory

    if (last_id)
    {
      snprintf(next_cursor_result, sizeof(next_cursor_result), "%s", last_id);
      free(last_id); // free the copy
    }
  }

  cJSON_AddItemToObject(json, "data", array);
  cJSON_AddNumberToObject(json, "count", count);
  cJSON_AddStringToObject(json, "next_cursor", next_cursor_result);

  char *str = cJSON_PrintUnformatted(json);

  mg_send_http_ok(conn, "application/json", strlen(str));
  mg_write(conn, str, strlen(str));

  // free the json string and json object
  cJSON_free(str);
  cJSON_Delete(json);
  return 200;
}

int get_dog_by_id_handler(struct mg_connection *conn, const struct mg_request_info *request)
{
  const char *uri = request->local_uri;
  const char *prefix = "/api/dogs/";
  size_t prefix_len = strlen(prefix);
  const char *dog_id = uri + prefix_len;
  if (!dog_id || strlen(dog_id) == 0)
  {
    mg_send_http_error(conn, 400, "Bad request");
    return 400;
  }

  if (!is_valid_uuid(dog_id))
  {
    mg_send_http_error(conn, 400, "Invalid dog_id format.");
    return 400;
  }

  PGconn *db_conn = db_pool_acquire();
  if (!db_conn)
  {
    mg_send_http_error(conn, 500, "DB connection couldn't established.");
    return 500;
  }

  Dog *dog;
  GetDogStatus status = get_dog_by_id(db_conn, dog_id, &dog);

  // Release db connection
  db_pool_release(db_conn);

  if (status == GET_DOG_ERROR)
  {
    mg_send_http_error(conn, 500, "Error retrieving dog by id.");
    return 500;
  }

  if (status == GET_DOG_NOT_FOUND)
  {
    mg_send_http_error(conn, 404, "Dog not found.");
    return 404;
  }

  cJSON *json = dog_to_json(dog);
  dog_free(dog);

  char *str = cJSON_PrintUnformatted(json);

  mg_send_http_ok(conn, "application/json", strlen(str));
  mg_write(conn, str, strlen(str));

  cJSON_free(str);
  cJSON_Delete(json);
  return 200;
}

int dogs_dispatcher_handler(struct mg_connection *conn, void *ignored)
{
  (void)ignored;

  const struct mg_request_info *req = mg_get_request_info(conn);
  const char *uri = req->local_uri;
  const char *method = req->request_method;

  if (strcmp(method, "GET") == 0 && strcmp(uri, "/api/dogs") == 0)
  {
    return get_all_dogs_handler(conn, req);
  }

  if (strcmp(method, "GET") == 0 && strncmp(uri, "/api/dogs/", 10) == 0 && strlen(uri) > 10)
  {
    return get_dog_by_id_handler(conn, req);
  }

  mg_printf(conn, "HTTP/1.1 404 Not Found\r\n\r\n");
  return 404;
}
