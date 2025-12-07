cmake_minimum_required(VERSION 3.15)
project(dogs_api C)

# -------------------------------------------------
# Optional vcpkg integration
# -------------------------------------------------
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# -------------------------------------------------
# C Standard
# -------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -------------------------------------------------
# Build type & compiler warnings
# -------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# -------------------------------------------------
# Find PostgreSQL
# -------------------------------------------------
find_package(PostgreSQL REQUIRED)

# ---- FIX FOR STATIC libpq ----
# libpgcommon.a and libpgport.a require math functions (log, sin, sqrt)
# PostgreSQL::PostgreSQL does NOT declare the dependency.
# Patch the imported target cleanly so CMake links correctly.
set_property(TARGET PostgreSQL::PostgreSQL APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES m
)

# -------------------------------------------------
# CivetWeb static library
# -------------------------------------------------
add_library(civetweb_lib STATIC src/vendors/civetweb/civetweb.c)
target_include_directories(civetweb_lib PUBLIC src/vendors/civetweb)
target_compile_definitions(civetweb_lib PRIVATE NO_SSL)

if(NOT MSVC)
    target_compile_options(civetweb_lib PRIVATE -w)
endif()

# -------------------------------------------------
# cJSON static library
# -------------------------------------------------
add_library(cjson STATIC src/vendors/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC src/vendors/cJSON)

if(NOT MSVC)
    target_compile_options(cjson PRIVATE -w)
endif()

# -------------------------------------------------
# Main executable
# -------------------------------------------------
add_executable(dogs_api
    src/main.c
    src/http/http.c
    src/http/router.c
    src/db/db.c
    src/helpers/uuid/uuid.c
    src/helpers/time_utils/time_utils.c
    src/logging/logging.c
    src/services/dog_service/dog_service.c
    src/services/db_health/db_health.c
    src/handlers/health_check.c
    src/handlers/get_dogs.c
)

target_include_directories(dogs_api PRIVATE
    src
    include
)

# Enable strptime, gmtime_r, and POSIX/XSI extensions
target_compile_definitions(dogs_api PRIVATE _XOPEN_SOURCE=700)

target_link_libraries(dogs_api PRIVATE
    civetweb_lib
    cjson
    PostgreSQL::PostgreSQL   # <-- now includes math dependency cleanly
    pthread
    dl
)

if(NOT MSVC)
    target_compile_options(dogs_api PRIVATE -Wno-newline-eof)
endif()
