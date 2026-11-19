#!/bin/sh
# Verification for the Dogs API:
#  1. cmocka unit-test suite (tests/test_dogs_api.c, built as dogs_api_tests,
#     registered with CTest in CMakeLists.txt).
#  2. Smoke test: starts the built server against the dev postgres and
#     exercises the API (the project's own compose healthcheck pattern:
#     curl -f /api/health).
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

if [ ! -x build/dogs_api ]; then
  echo "ERROR: build/dogs_api not found — run the build step first." >&2
  exit 1
fi

# --- unit tests (cmocka / CTest) --------------------------------------------
echo "==> Running unit tests (ctest)"
ctest --test-dir build --output-on-failure

export DB_HOST="${DB_HOST:-postgres}"
export DB_PORT="${DB_PORT:-5432}"
export DB_USER="${DB_USER:-postgres}"
export DB_PASSWORD="${DB_PASSWORD:-dev}"
export DB_NAME="${DB_NAME:-postgres}"

./build/dogs_api > /tmp/dogs_api.log 2>&1 &
SERVER_PID=$!

cleanup() {
  kill "$SERVER_PID" 2>/dev/null || true
  wait "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# --- wait for the server to answer -----------------------------------------
i=0
until curl -fsS http://127.0.0.1:8080/api/health > /dev/null 2>&1; do
  i=$((i + 1))
  if [ "$i" -ge 30 ]; then
    echo "ERROR: server did not become healthy in time" >&2
    cat /tmp/dogs_api.log >&2 || true
    exit 1
  fi
  sleep 1
done

echo "==> GET /api/health"
curl -fsS http://127.0.0.1:8080/api/health
echo
echo "==> GET /api/dogs?limit=3"
curl -fsS "http://127.0.0.1:8080/api/dogs?limit=3"
echo
echo "SMOKE TEST PASSED"
