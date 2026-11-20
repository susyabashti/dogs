#include "logging/logging.h"
#include "cJSON.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "dog_service.h"
#include "db/db.h"
#include "time.h"
#include "helpers/time_utils/time_utils.h"

Dog *dog_new(const char *id,
             const char *breed,
             const char *country_origin,
             const char *fur_color,
             const char *eye_colors,
             int min_height,
             int max_height,
             int min_longevity,
             int max_longevity,
             const char *character_traits,
             const char *common_health_problems,
             time_t created_at)
{
  Dog *d = malloc(sizeof(Dog));

  d->id = strdup(id);
  d->breed = strdup(breed);
  d->country_origin = strdup(country_origin);
  d->fur_color = strdup(fur_color);
  d->eye_colors = strdup(eye_colors);
  d->character_traits = strdup(character_traits);
  d->common_health_problems = strdup(common_health_problems);
  d->min_height = min_height;
  d->max_height = max_height;
  d->min_longevity = min_longevity;
  d->max_longevity = max_longevity;
  d->created_at = created_at;

  return d;
}

void dog_free(Dog *d)
{
  if (!d)
    return;

  free(d->id);
  free(d->breed);
  free(d->country_origin);
  free(d->fur_color);
  free(d->eye_colors);
  free(d->character_traits);
  free(d->common_health_problems);

  free(d);
}

Dog *dog_from_pg_result(PGresult *result, int row_num)
{
  const char *creadet_at_str = PQgetvalue(result, row_num, 11);
  struct tm tm;
  strptime(creadet_at_str, "%Y-%m-%d %H:%M:%S", &tm);
  time_t created_at = portable_timegm(&tm);

  return dog_new(PQgetvalue(result, row_num, 0),
                 PQgetvalue(result, row_num, 1),
                 PQgetvalue(result, row_num, 2),
                 PQgetvalue(result, row_num, 3),
                 PQgetvalue(result, row_num, 4),
                 atoi(PQgetvalue(result, row_num, 5)),
                 atoi(PQgetvalue(result, row_num, 6)),
                 atoi(PQgetvalue(result, row_num, 7)),
                 atoi(PQgetvalue(result, row_num, 8)),
                 PQgetvalue(result, row_num, 9),
                 PQgetvalue(result, row_num, 10),
                 created_at);
}

cJSON *dog_to_json(Dog *dog)
{
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "id", dog->id);
  cJSON_AddStringToObject(json, "breed", dog->breed);
  cJSON_AddStringToObject(json, "country_origin", dog->country_origin);
  cJSON_AddStringToObject(json, "fur_color", dog->fur_color);
  cJSON_AddStringToObject(json, "eye_colors", dog->eye_colors);
  cJSON_AddNumberToObject(json, "min_height", dog->min_height);
  cJSON_AddNumberToObject(json, "max_height", dog->max_height);
  cJSON_AddNumberToObject(json, "min_longevity", dog->min_longevity);
  cJSON_AddNumberToObject(json, "max_longevity", dog->max_longevity);
  cJSON_AddStringToObject(json, "character_traits", dog->character_traits);
  cJSON_AddStringToObject(json, "common_health_problems", dog->common_health_problems);

  char buf[32];
  struct tm tm;
  gmtime_r(&dog->created_at, &tm);
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  cJSON_AddStringToObject(json, "created_at", buf);

  return json;
}

int dog_service_resolve_limit(const char *limit_str)
{
  if (!limit_str || strlen(limit_str) == 0)
    return DOGS_DEFAULT_LIMIT;

  // strtol (not atoi) so out-of-range input is detected instead of
  // invoking undefined behavior on overflow.
  errno = 0;
  char *end = NULL;
  long limit = strtol(limit_str, &end, 10);

  if (end == limit_str || *end != '\0' || errno == ERANGE)
    return DOGS_DEFAULT_LIMIT;

  if (limit <= 0)
    return DOGS_DEFAULT_LIMIT;
  if (limit > DOGS_MAX_LIMIT)
    return DOGS_MAX_LIMIT;

  return (int)limit;
}

cJSON *dog_service_build_list_response(Dog **dogs, size_t *out_count)
{
  cJSON *json = cJSON_CreateObject();
  cJSON *array = cJSON_CreateArray();

  size_t count = 0;
  char next_cursor[37] = "";

  if (dogs)
  {
    for (int i = 0; dogs[i] != NULL; i++)
    {
      cJSON *dog_json = dog_to_json(dogs[i]);
      cJSON_AddItemToArray(array, dog_json);
      count++;

      // Copy the id before the dog is freed; the last row wins.
      snprintf(next_cursor, sizeof(next_cursor), "%s", dogs[i]->id);
      dog_free(dogs[i]);
    }
    free(dogs);
  }

  cJSON_AddItemToObject(json, "data", array);
  cJSON_AddNumberToObject(json, "count", (double)count);
  cJSON_AddStringToObject(json, "next_cursor", next_cursor);

  if (out_count)
    *out_count = count;

  return json;
}

void register_dog_service_prepared_statements(void)
{
  db_register_prepared("get_dogs_by_query", "SELECT * FROM dogs WHERE id > $1 ORDER BY id ASC LIMIT $2");
  db_register_prepared("get_dog_by_id", "SELECT * FROM dogs WHERE id = $1");
}

GetDogsByQueryStatus get_dogs_by_query(PGconn *db_conn, const char *cursor, int limit, Dog ***out_dogs)
{
  *out_dogs = NULL;

  const char *paramValues[2];

  char limit_str[16];
  snprintf(limit_str, sizeof(limit_str), "%d", limit);

  paramValues[0] = cursor;
  paramValues[1] = limit_str;

  PGresult *result = PQexecPrepared(db_conn, "get_dogs_by_query", 2, paramValues, NULL, NULL, 0);
  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    log_msg(LOG_ERROR, "DB query failed for cursor=%s: %s", cursor, PQerrorMessage(db_conn));
    PQclear(result);
    return GET_DOGS_BY_QUERY_ERROR;
  }

  int rows = PQntuples(result);
  if (rows == 0)
  {
    PQclear(result);
    return GET_DOGS_BY_QUERY_EMPTY;
  }

  Dog **dogs = malloc((rows + 1) * sizeof(Dog *));

  for (int i = 0; i < rows; i++)
  {
    dogs[i] = dog_from_pg_result(result, i);
  }

  dogs[rows] = NULL;

  PQclear(result);
  *out_dogs = dogs;

  return GET_DOGS_BY_QUERY_OK;
}

GetDogStatus get_dog_by_id(PGconn *db_conn, const char *dog_id, Dog **out_dog)
{
  *out_dog = NULL;

  const char *paramValues[1];
  paramValues[0] = dog_id;

  PGresult *result = PQexecPrepared(db_conn, "get_dog_by_id", 1, paramValues, NULL, NULL, 0);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    log_msg(LOG_ERROR, "DB query failed for dog_id=%s: %s", dog_id, PQerrorMessage(db_conn));
    PQclear(result);
    return GET_DOG_ERROR;
  }

  int rows = PQntuples(result);
  if (rows == 0)
  {
    PQclear(result);
    return GET_DOG_NOT_FOUND;
  }

  // Convert row to Dog struct
  *out_dog = dog_from_pg_result(result, 0);
  PQclear(result);

  return GET_DOG_OK;
}