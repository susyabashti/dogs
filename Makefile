# =========================================================
# Project Makefile
# =========================================================

# Build directory
BUILD_DIR := build

# Docker Compose file
DOCKER_COMPOSE := docker-compose.yaml

# CMake executable
CMAKE := cmake

# Optional vcpkg
VCPKG_TOOLCHAIN :=
ifeq ($(VCPKG_ROOT),)
  VCPKG_TOOLCHAIN :=
else
  VCPKG_TOOLCHAIN := -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
endif

# Colors
GREEN := \033[0;32m
NC    := \033[0m

# =========================================================
# Phony targets
# =========================================================
.PHONY: all build clean run docker-up docker-down docker-logs docker-rebuild docker-enter

# Default target
all: build

# ---------------------------------------------------------
# Build C project
# ---------------------------------------------------------
build:
	@echo "$(GREEN)Building project in $(BUILD_DIR)...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. $(VCPKG_TOOLCHAIN)
	@$(MAKE) -C $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "$(GREEN)Cleaning build directory...$(NC)"
	@rm -rf $(BUILD_DIR)

# Run the binary (load .env automatically)
run:
	@echo "$(GREEN)Running dogs_api with environment variables from .env...$(NC)"
	@set -a; \
	. .env; \
	set +a; \
	$(BUILD_DIR)/dogs_api

# ---------------------------------------------------------
# Docker Compose commands
# ---------------------------------------------------------

docker-up:
	@echo "$(GREEN)Starting Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) up -d

docker-down:
	@echo "$(GREEN)Stopping Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) down

docker-rebuild:
	@echo "$(GREEN)Rebuilding Docker Compose services...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) build --no-cache

docker-logs:
	@echo "$(GREEN)Attaching to Docker Compose logs...$(NC)"
	@docker compose -f $(DOCKER_COMPOSE) logs -f

docker-enter:
	@echo "$(GREEN)Entering dogs_api container...$(NC)"
	@docker exec -it dogs_api /bin/sh
