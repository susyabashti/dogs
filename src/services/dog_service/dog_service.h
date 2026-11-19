#pragma once

#include "db/db.h"
#include "cJSON.h"
#include <stddef.h>

// Pagination limits for /api/dogs
#define DOGS_DEFAULT_LIMIT 10
#define DOGS_MAX_LIMIT 25

typedef struct
{
  char *id;
  char *breed;
  char *country_origin;
  char *fur_color;
  char *eye_colors;
  char *character_traits;
  char *common_health_problems;
  int min_height;
  int max_height;
  int min_longevity;
  int max_longevity;
  time_t created_at;
} Dog;

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
             time_t created_at);

void dog_free(Dog *d);

void register_dog_service_prepared_statements(void);

cJSON *dog_to_json(Dog *dog);

// Resolve the effective page size from a raw 'limit' query parameter.
// NULL/empty/non-positive/invalid input falls back to DOGS_DEFAULT_LIMIT;
// values above DOGS_MAX_LIMIT are capped at DOGS_MAX_LIMIT.
int dog_service_resolve_limit(const char *limit_str);

// Build the /api/dogs list response object: { data: [...], count: N, next_cursor: "..." }.
// Consumes the NULL-terminated dog list (frees every Dog and the array itself).
// Pass NULL for an empty page. *out_count (optional) receives the number of dogs.
cJSON *dog_service_build_list_response(Dog **dogs, size_t *out_count);

typedef enum
{
  GET_DOGS_BY_QUERY_OK = 0,
  GET_DOGS_BY_QUERY_EMPTY,
  GET_DOGS_BY_QUERY_ERROR
} GetDogsByQueryStatus;

GetDogsByQueryStatus get_dogs_by_query(PGconn *db_conn, const char *next_cursor, int limit, Dog ***out_dogs);

typedef enum
{
  GET_DOG_OK = 0,
  GET_DOG_NOT_FOUND,
  GET_DOG_ERROR
} GetDogStatus;

GetDogStatus get_dog_by_id(PGconn *db_conn, const char *dog_id, Dog **dog_out);