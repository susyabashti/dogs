# =========================================================
# Project Makefile
# =========================================================

# Docker Compose file
DOCKER_COMPOSE := docker-compose.yaml


# Colors
GREEN := \033[0;32m
NC    := \033[0m

# =========================================================
# Phony targets
# =========================================================
.PHONY: docker-up docker-down docker-logs docker-rebuild docker-enter format format-check format-guard lint lint-cppcheck lint-guard

# clang-format
CLANG_FORMAT := clang-format
# All C sources/headers under src/, excluding vendored code
FORMAT_FILES := $(shell find src -type f \( -name '*.c' -o -name '*.h' \) -not -path 'src/vendors/*')

# Default target
all: docker-up

# ---------------------------------------------------------
# Docker Compose commands
# ---------------------------------------------------------

docker-up:
	@echo "$(GREEN)Starting Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) up -d

docker-down:
	@echo "$(GREEN)Stopping Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) down

docker-destroy:
	@echo "$(GREEN)Stopping Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) down -v

docker-build:
	@echo "$(GREEN)Rebuilding Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) build

docker-build-clean:
	@echo "$(GREEN)Rebuilding Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) build --no-cache

docker-logs:
	@echo "$(GREEN)Attaching to Docker Compose logs...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) logs -f

docker-enter:
	@echo "$(GREEN)Entering dogs_api container...$(NC)"
	@docker exec -it dogs_api /bin/sh

# ---------------------------------------------------------
# Code formatting (clang-format, style in .clang-format)
# ---------------------------------------------------------

# Guard: without any files, 'clang-format -i' would hang reading stdin.
format-guard:
	@if [ -z "$(FORMAT_FILES)" ]; then \
	  echo "error: no C sources found under src/ - run make from the repository root" >&2; \
	  exit 1; \
	fi
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "error: clang-format not found in PATH (install it, e.g. 'apt install clang-format')" >&2; exit 1; }

format: format-guard
	@echo "$(GREEN)Formatting src/ (excluding vendors)...$(NC)"
	@$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check: format-guard
	@echo "$(GREEN)Checking formatting of src/ (excluding vendors)...$(NC)"
	@$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

# ---------------------------------------------------------
# Static analysis (clang-tidy + cppcheck), project code only
# ---------------------------------------------------------

CLANG_TIDY := clang-tidy
CPPCHECK   := cppcheck

# All project translation units; vendored code is never linted.
LINT_SOURCES := $(shell find src tests -type f -name '*.c' -not -path 'src/vendors/*')

# Mirror the CMake compile flags (C standard, defines, src + vendored + libpq + cmocka includes).
LINT_CFLAGS_LIBPQ  := $(shell pkg-config --cflags-only-I libpq 2>/dev/null)
LINT_CFLAGS_CMOCKA := $(shell pkg-config --cflags-only-I cmocka 2>/dev/null)
LINT_CFLAGS        := -std=c11 -D_XOPEN_SOURCE=700 -Isrc -Isrc/vendors/civetweb -Isrc/vendors/cJSON $(LINT_CFLAGS_LIBPQ) $(LINT_CFLAGS_CMOCKA)

lint-guard:
	@if [ -z "$(LINT_SOURCES)" ]; then \
	  echo "error: no C sources found under src/ - run make from the repository root" >&2; \
	  exit 1; \
	fi
	@command -v $(CLANG_TIDY) >/dev/null 2>&1 || { echo "error: clang-tidy not found in PATH (install it, e.g. 'apt install clang-tidy')" >&2; exit 1; }
	@command -v $(CPPCHECK) >/dev/null 2>&1 || { echo "error: cppcheck not found in PATH (install it, e.g. 'apt install cppcheck')" >&2; exit 1; }

# clang-tidy per translation unit (config in .clang-tidy; WarningsAsErrors makes
# any finding fail the build).
lint: lint-guard
	@echo "$(GREEN)Running clang-tidy on project sources (excluding vendors)...$(NC)"
	@for f in $(LINT_SOURCES); do \
	  echo "[clang-tidy] $$f"; \
	  $(CLANG_TIDY) --quiet $$f -- $(LINT_CFLAGS) || exit 1; \
	done
	@echo "$(GREEN)clang-tidy: clean$(NC)"

# cppcheck notes:
#  -i src/vendors      vendored code must not produce any reports
#  --suppress=missingInclude
#                     this environment's cppcheck cannot resolve standard C
#                     headers (stdio.h & co); the note only ever concerns system
#                     headers, which cppcheck does not need for correct results
#  ConfigurationNotChecked scoped to civetweb.h
#                     civetweb conditionally defines MG_MATCH_CONTEXT_MAX_MATCHES;
#                     keep vendor noise out of the report
lint-cppcheck: lint-guard
	@echo "$(GREEN)Running cppcheck on src/ (excluding vendors)...$(NC)"
	@$(CPPCHECK) --enable=all --std=c11 --inline-suppr --force \
	  -isrc/vendors \
	  -Isrc -Isrc/vendors/civetweb -Isrc/vendors/cJSON $(LINT_CFLAGS_LIBPQ) \
	  --suppress=missingInclude \
	  --suppress=ConfigurationNotChecked:src/vendors/civetweb/civetweb.h \
	  --error-exitcode=1 src/
	@echo "$(GREEN)cppcheck: clean$(NC)"
