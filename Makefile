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
.PHONY: docker-up docker-down docker-logs docker-rebuild docker-enter format format-check format-guard

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
