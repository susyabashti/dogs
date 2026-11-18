#!/bin/sh
# Environment setup for the Dogs API dev container.
#  1. Fetches the vendored cJSON dependency that CMakeLists.txt expects at
#     src/vendors/cJSON/ but which is missing from the repository.
#  2. Waits for the postgres service.
#  3. Applies the project's own schema + seed (zarf/docker/postgres/init).
# Idempotent: safe to re-run.
set -eu

# --- locate the repository root -------------------------------------------
REPO_ROOT=""
for d in "$PWD" "$PWD/.." /workspace /app /home/legion; do
  if [ -f "$d/CMakeLists.txt" ] && [ -d "$d/src" ]; then
    REPO_ROOT="$d"
    break
  fi
done
if [ -z "$REPO_ROOT" ]; then
  echo "ERROR: could not locate repository root (CMakeLists.txt + src/)" >&2
  exit 1
fi
cd "$REPO_ROOT"
echo "==> Repo root: $REPO_ROOT"

# --- fetch missing vendored dependency: cJSON v1.7.19 ----------------------
if [ ! -f src/vendors/cJSON/cJSON.c ] || [ ! -f src/vendors/cJSON/cJSON.h ]; then
  echo "==> Fetching vendored cJSON v1.7.19 into src/vendors/cJSON/ ..."
  mkdir -p src/vendors/cJSON /tmp/cjson-dl
  curl -fsSL https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.19.tar.gz \
       -o /tmp/cjson-dl/cjson.tar.gz
  tar -xzf /tmp/cjson-dl/cjson.tar.gz -C /tmp/cjson-dl
  cp /tmp/cjson-dl/cJSON-1.7.19/cJSON.c src/vendors/cJSON/cJSON.c
  cp /tmp/cjson-dl/cJSON-1.7.19/cJSON.h src/vendors/cJSON/cJSON.h
  rm -rf /tmp/cjson-dl
else
  echo "==> cJSON already present, skipping fetch."
fi

# --- wait for postgres ------------------------------------------------------
DB_HOST="${DB_HOST:-postgres}"
DB_PORT="${DB_PORT:-5432}"
DB_USER="${DB_USER:-postgres}"
DB_PASSWORD="${DB_PASSWORD:-dev}"
DB_NAME="${DB_NAME:-postgres}"

echo "==> Waiting for postgres at $DB_HOST:$DB_PORT ..."
i=0
until pg_isready -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -q; do
  i=$((i + 1))
  if [ "$i" -ge 60 ]; then
    echo "ERROR: postgres did not become ready in time" >&2
    exit 1
  fi
  sleep 2
done

# --- schema + seed (the project's own init scripts) -------------------------
export PGPASSWORD="$DB_PASSWORD"

echo "==> Applying schema (zarf/docker/postgres/init/01_schema.sql) ..."
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" \
     -v ON_ERROR_STOP=1 -f zarf/docker/postgres/init/01_schema.sql

echo "==> Seeding dogs table from zarf/docker/postgres/init/data.csv ..."
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 <<EOF
TRUNCATE dogs;
\\copy dogs(breed, country_origin, fur_color, eye_colors, character_traits, common_health_problems, min_height, max_height, min_longevity, max_longevity) FROM '$REPO_ROOT/zarf/docker/postgres/init/data.csv' DELIMITER ',' CSV HEADER
EOF

echo "==> Setup complete."
