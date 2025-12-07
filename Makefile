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
.PHONY: docker-up docker-down docker-logs docker-rebuild docker-enter

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
