#pragma once

#include "db/db.h"
#include "cJSON.h"

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
  GET_DOG_ERROR,
} GetDogStatus;

GetDogStatus get_dog_by_id(PGconn *db_conn, const char *dog_id, Dog **dog_out);