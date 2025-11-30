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
if(NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL client library not found. Please install it (brew install postgresql on macOS, apt install libpq-dev on Linux).")
endif()

# -------------------------------------------------
# CivetWeb static library
# -------------------------------------------------
add_library(civetweb_lib STATIC src/vendors/civetweb/civetweb.c)
target_include_directories(civetweb_lib PUBLIC src/vendors/civetweb)
target_compile_definitions(civetweb_lib PRIVATE NO_SSL NO_WEBSOCKET NO_FILES)

# Disable warnings for CivetWeb (keep -Werror for your code)
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
    src/db_health/db_health.c
    src/handlers/health_check.c
    src/handlers/get_dogs.c
)

target_include_directories(dogs_api PRIVATE
    src
    include
)

target_link_libraries(dogs_api PRIVATE
    civetweb_lib
    PostgreSQL::PostgreSQL
    cjson
)

# -------------------------------------------------
# System libraries
# -------------------------------------------------
if(UNIX)
    target_link_libraries(dogs_api PRIVATE pthread)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(dogs_api PRIVATE dl)
endif()

# -------------------------------------------------
# Optional: suppress EOF newline warnings (if any)
# -------------------------------------------------
if(NOT MSVC)
    target_compile_options(dogs_api PRIVATE -Wno-newline-eof)
endif()
