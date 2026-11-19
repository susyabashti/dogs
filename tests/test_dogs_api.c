#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "helpers/uuid/uuid.h"
#include "helpers/time_utils/time_utils.h"
#include "services/dog_service/dog_service.h"

// -------------------------------------------------
// Helpers
// -------------------------------------------------

static Dog *make_dog(const char *id, const char *breed)
{
  return dog_new(id,
                 breed,
                 "Germany",
                 "Black",
                 "Brown",
                 50,
                 60,
                 10,
                 13,
                 "Loyal, protective",
                 "Hip dysplasia",
                 1700000000);
}

static void assert_string_field(cJSON *json, const char *name, const char *expected)
{
  cJSON *item = cJSON_GetObjectItem(json, name);
  assert_non_null(item);
  assert_true(cJSON_IsString(item));
  assert_string_equal(item->valuestring, expected);
}

static void assert_number_field(cJSON *json, const char *name, int expected)
{
  cJSON *item = cJSON_GetObjectItem(json, name);
  assert_non_null(item);
  assert_true(cJSON_IsNumber(item));
  assert_int_equal(item->valueint, expected);
}

// -------------------------------------------------
// UUID validation
// -------------------------------------------------

static void test_uuid_valid_v4(void **state)
{
  (void)state;
  assert_true(is_valid_uuid("123e4567-e89b-42d3-a456-426614174000"));
  assert_true(is_valid_uuid("00000000-0000-0000-0000-000000000000"));
  assert_true(is_valid_uuid("9f8e7d6c-5b4a-3928-1716-050403020100"));
}

static void test_uuid_null_and_empty(void **state)
{
  (void)state;
  assert_false(is_valid_uuid(NULL));
  assert_false(is_valid_uuid(""));
}

static void test_uuid_wrong_length(void **state)
{
  (void)state;
  // 35 chars (one digit short)
  assert_false(is_valid_uuid("123e4567-e89b-42d3-a456-42661417400"));
  // 37 chars (one digit extra)
  assert_false(is_valid_uuid("123e4567-e89b-42d3-a456-4266141740001"));
  // 32 chars (no dashes at all)
  assert_false(is_valid_uuid("123e4567e89b42d3a456426614174000"));
}

static void test_uuid_bad_charset(void **state)
{
  (void)state;
  // non-hex letter
  assert_false(is_valid_uuid("123e4567-e89b-42d3-a456-42661417400z"));
  // non-hex letter at the start
  assert_false(is_valid_uuid("g23e4567-e89b-42d3-a456-426614174000"));
  // underscore instead of hex digit
  assert_false(is_valid_uuid("123e4567-e89b-42d3-a456-4266141740_0"));
  // space inside
  assert_false(is_valid_uuid("123e4567-e89b-42d3-a456-426614174 00"));
}

static void test_uuid_dash_positions(void **state)
{
  (void)state;
  // missing a dash (35 chars)
  assert_false(is_valid_uuid("123e4567e89b-42d3-a456-426614174000"));
  // dash in the wrong place
  assert_false(is_valid_uuid("123e4567-e89b4-42d3-a456-426614174000"));
  // extra dash (37 chars)
  assert_false(is_valid_uuid("123e4567-e89b-42d3-a456-4266-14174000"));
}

static void test_uuid_case_behavior(void **state)
{
  (void)state;
  // isxdigit-based validation: both cases are accepted
  assert_true(is_valid_uuid("123E4567-E89B-42D3-A456-426614174000"));
  assert_true(is_valid_uuid("123e4567-e89b-42d3-a456-426614174000"));
  assert_true(is_valid_uuid("AbCdEf01-2345-6789-aBcD-Ef0123456789"));
}

// -------------------------------------------------
// time_utils: portable_timegm
// -------------------------------------------------

static void test_timegm_epoch(void **state)
{
  (void)state;
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  tm.tm_year = 70; // years since 1900
  tm.tm_mon = 0;
  tm.tm_mday = 1;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;

  assert_int_equal(portable_timegm(&tm), 0);
}

static void test_timegm_known_timestamps(void **state)
{
  (void)state;
  struct tm tm;
  memset(&tm, 0, sizeof(tm));

  // 2001-09-09 01:46:40 UTC
  tm.tm_year = 101;
  tm.tm_mon = 8;
  tm.tm_mday = 9;
  tm.tm_hour = 1;
  tm.tm_min = 46;
  tm.tm_sec = 40;
  assert_int_equal(portable_timegm(&tm), 1000000000);

  // 2023-11-14 22:13:20 UTC
  tm.tm_year = 123;
  tm.tm_mon = 10;
  tm.tm_mday = 14;
  tm.tm_hour = 22;
  tm.tm_min = 13;
  tm.tm_sec = 20;
  assert_int_equal(portable_timegm(&tm), 1700000000);
}

static void test_timegm_roundtrip_against_gmtime_r(void **state)
{
  (void)state;
  const time_t known[] = {
      0,
      1,
      86399,
      1000000000,
      1577836800,
      1700000000,
      4102444800,
  };

  for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++)
  {
    struct tm tm;
    gmtime_r(&known[i], &tm);
    time_t roundtrip = portable_timegm(&tm);
    assert_int_equal(roundtrip, known[i]);
  }
}

static void test_timegm_independent_of_local_tz(void **state)
{
  (void)state;
  const time_t t = 1700000000; // 2023-11-14 22:13:20 UTC

  char *old_tz = getenv("TZ");
  char old_tz_buf[256] = "";
  if (old_tz)
    snprintf(old_tz_buf, sizeof(old_tz_buf), "%s", old_tz);

  // Force a non-UTC local timezone; portable_timegm must still return UTC.
  setenv("TZ", "America/New_York", 1);
  tzset();

  struct tm tm;
  gmtime_r(&t, &tm);
  time_t result = portable_timegm(&tm);

  if (old_tz)
    setenv("TZ", old_tz_buf, 1);
  else
    unsetenv("TZ");
  tzset();

  assert_int_equal(result, t);
}

// -------------------------------------------------
// Pagination: limit resolution
// -------------------------------------------------

static void test_limit_defaults(void **state)
{
  (void)state;
  assert_int_equal(dog_service_resolve_limit(NULL), DOGS_DEFAULT_LIMIT);
  assert_int_equal(dog_service_resolve_limit(""), DOGS_DEFAULT_LIMIT);
  assert_int_equal(dog_service_resolve_limit("0"), DOGS_DEFAULT_LIMIT);
  assert_int_equal(dog_service_resolve_limit("-3"), DOGS_DEFAULT_LIMIT);
  assert_int_equal(dog_service_resolve_limit("abc"), DOGS_DEFAULT_LIMIT);
}

static void test_limit_passthrough(void **state)
{
  (void)state;
  assert_int_equal(dog_service_resolve_limit("1"), 1);
  assert_int_equal(dog_service_resolve_limit("10"), 10);
  assert_int_equal(dog_service_resolve_limit("25"), 25);
}

static void test_limit_capped_at_max(void **state)
{
  (void)state;
  assert_int_equal(dog_service_resolve_limit("26"), DOGS_MAX_LIMIT);
  assert_int_equal(dog_service_resolve_limit("1000"), DOGS_MAX_LIMIT);
  assert_int_equal(dog_service_resolve_limit("999999999"), DOGS_MAX_LIMIT);
}

// -------------------------------------------------
// Pagination: list response (cursor + shape)
// -------------------------------------------------

static void test_response_shape_and_cursor(void **state)
{
  (void)state;
  Dog **dogs = malloc(3 * sizeof(Dog *));
  dogs[0] = make_dog("11111111-1111-4111-8111-111111111111", "Breed A");
  dogs[1] = make_dog("22222222-2222-4222-8222-222222222222", "Breed B");
  dogs[2] = NULL;

  size_t count = 99;
  cJSON *json = dog_service_build_list_response(dogs, &count);
  assert_non_null(json);
  assert_true(cJSON_IsObject(json));

  // count
  assert_int_equal(count, 2);
  cJSON *count_item = cJSON_GetObjectItem(json, "count");
  assert_non_null(count_item);
  assert_true(cJSON_IsNumber(count_item));
  assert_int_equal(count_item->valueint, 2);

  // data: array of 2 dog objects
  cJSON *data = cJSON_GetObjectItem(json, "data");
  assert_non_null(data);
  assert_true(cJSON_IsArray(data));
  assert_int_equal(cJSON_GetArraySize(data), 2);

  // first item keeps the dog field names
  cJSON *first = cJSON_GetArrayItem(data, 0);
  assert_non_null(first);
  assert_string_field(first, "id", "11111111-1111-4111-8111-111111111111");
  assert_string_field(first, "breed", "Breed A");

  // next_cursor equals the last row's UUID
  cJSON *cursor = cJSON_GetObjectItem(json, "next_cursor");
  assert_non_null(cursor);
  assert_true(cJSON_IsString(cursor));
  assert_string_equal(cursor->valuestring, "22222222-2222-4222-8222-222222222222");

  cJSON_Delete(json);
}

static void test_response_single_row_cursor(void **state)
{
  (void)state;
  Dog **dogs = malloc(2 * sizeof(Dog *));
  dogs[0] = make_dog("33333333-3333-4333-8333-333333333333", "Solo");
  dogs[1] = NULL;

  size_t count = 0;
  cJSON *json = dog_service_build_list_response(dogs, &count);
  assert_non_null(json);

  assert_int_equal(count, 1);

  cJSON *cursor = cJSON_GetObjectItem(json, "next_cursor");
  assert_non_null(cursor);
  assert_string_equal(cursor->valuestring, "33333333-3333-4333-8333-333333333333");

  cJSON_Delete(json);
}

static void test_response_empty_page_has_no_cursor(void **state)
{
  (void)state;
  size_t count = 99;
  cJSON *json = dog_service_build_list_response(NULL, &count);
  assert_non_null(json);
  assert_true(cJSON_IsObject(json));

  assert_int_equal(count, 0);

  cJSON *data = cJSON_GetObjectItem(json, "data");
  assert_non_null(data);
  assert_true(cJSON_IsArray(data));
  assert_int_equal(cJSON_GetArraySize(data), 0);

  cJSON *count_item = cJSON_GetObjectItem(json, "count");
  assert_non_null(count_item);
  assert_true(cJSON_IsNumber(count_item));
  assert_int_equal(count_item->valueint, 0);

  // Empty page must not carry a usable cursor
  cJSON *cursor = cJSON_GetObjectItem(json, "next_cursor");
  if (cursor)
  {
    assert_true(cJSON_IsString(cursor));
    assert_int_equal(strlen(cursor->valuestring), 0);
  }

  cJSON_Delete(json);
}

// -------------------------------------------------
// JSON response shape for /api/dogs dog objects
// -------------------------------------------------

static void test_dog_json_field_names_and_types(void **state)
{
  (void)state;
  Dog *dog = make_dog("44444444-4444-4444-8444-444444444444", "Rottweiler");

  cJSON *json = dog_to_json(dog);
  assert_non_null(json);
  assert_true(cJSON_IsObject(json));

  assert_string_field(json, "id", "44444444-4444-4444-8444-444444444444");
  assert_string_field(json, "breed", "Rottweiler");
  assert_string_field(json, "country_origin", "Germany");
  assert_string_field(json, "fur_color", "Black");
  assert_string_field(json, "eye_colors", "Brown");
  assert_number_field(json, "min_height", 50);
  assert_number_field(json, "max_height", 60);
  assert_number_field(json, "min_longevity", 10);
  assert_number_field(json, "max_longevity", 13);
  assert_string_field(json, "character_traits", "Loyal, protective");
  assert_string_field(json, "common_health_problems", "Hip dysplasia");
  // 1700000000 == 2023-11-14 22:13:20 UTC
  assert_string_field(json, "created_at", "2023-11-14T22:13:20Z");

  cJSON_Delete(json);
  dog_free(dog);
}

// -------------------------------------------------
// Runner
// -------------------------------------------------

int main(void)
{
  const struct CMUnitTest tests[] =
  {
      cmocka_unit_test(test_uuid_valid_v4),
      cmocka_unit_test(test_uuid_null_and_empty),
      cmocka_unit_test(test_uuid_wrong_length),
      cmocka_unit_test(test_uuid_bad_charset),
      cmocka_unit_test(test_uuid_dash_positions),
      cmocka_unit_test(test_uuid_case_behavior),
      cmocka_unit_test(test_timegm_epoch),
      cmocka_unit_test(test_timegm_known_timestamps),
      cmocka_unit_test(test_timegm_roundtrip_against_gmtime_r),
      cmocka_unit_test(test_timegm_independent_of_local_tz),
      cmocka_unit_test(test_limit_defaults),
      cmocka_unit_test(test_limit_passthrough),
      cmocka_unit_test(test_limit_capped_at_max),
      cmocka_unit_test(test_response_shape_and_cursor),
      cmocka_unit_test(test_response_single_row_cursor),
      cmocka_unit_test(test_response_empty_page_has_no_cursor),
      cmocka_unit_test(test_dog_json_field_names_and_types),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
