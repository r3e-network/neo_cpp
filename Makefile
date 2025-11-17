# Neo C++ Project Makefile
# Comprehensive build system for Neo blockchain implementation

# =============================================================================
# Configuration Variables
# =============================================================================

PROJECT_NAME := neo-cpp
VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "dev")
COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_DATE := $(shell date '+%Y-%m-%d %H:%M:%S')
BUILD_DIR := build
DOCKER_IMAGE := neo-cpp
DOCKER_TAG := $(VERSION)

# Tools
CMAKE := cmake
MAKE := make
CTEST := ctest
DOCKER := docker
CLANG_FORMAT := clang-format
CLANG_TIDY := clang-tidy

# Build configuration
BUILD_TYPE ?= Release
JOBS ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
VERBOSE ?= 0
INSTALL_PREFIX ?= /usr/local

# Feature flags
ENABLE_TESTS ?= ON
ENABLE_EXAMPLES ?= ON
ENABLE_TOOLS ?= ON
ENABLE_APPS ?= ON
ENABLE_COVERAGE ?= OFF
ENABLE_ASAN ?= OFF
ENABLE_TSAN ?= OFF

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
endif
ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
endif

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
PURPLE := \033[0;35m
CYAN := \033[0;36m
WHITE := \033[0;37m
BOLD := \033[1m
NC := \033[0m # No Color

# =============================================================================
# Default Target
# =============================================================================

.PHONY: all
all: build

.DEFAULT_GOAL := all

# =============================================================================
# Help Documentation
# =============================================================================

.PHONY: help
help:
	@echo "$(BOLD)$(CYAN)Neo C++ Build System$(NC)"
	@echo "$(WHITE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo ""
	@echo "$(BOLD)$(YELLOW)Basic Commands:$(NC)"
	@echo "  $(GREEN)make$(NC)              - Build the project ($(BUILD_TYPE) mode)"
	@echo "  $(GREEN)make debug$(NC)        - Build in Debug mode"
	@echo "  $(GREEN)make release$(NC)      - Build in Release mode"
	@echo "  $(GREEN)make clean$(NC)        - Clean build artifacts"
	@echo "  $(GREEN)make distclean$(NC)    - Remove all generated files"
	@echo "  $(GREEN)make install$(NC)      - Install to $(INSTALL_PREFIX)"
	@echo ""
	@echo "$(BOLD)$(YELLOW)Testing & Quality:$(NC)"
	@echo "  $(GREEN)make test$(NC)         - Run all tests"
	@echo "  $(GREEN)make test-unit$(NC)    - Run unit tests only"
	@echo "  $(GREEN)make coverage$(NC)     - Generate test coverage report"
	@echo "  $(GREEN)make check$(NC)        - Run static analysis"
	@echo "  $(GREEN)make format$(NC)       - Format code with clang-format"
	@echo "  $(GREEN)make tidy$(NC)         - Run clang-tidy"
	@echo ""
	@echo "$(BOLD)$(YELLOW)Running Neo:$(NC)"
	@echo "  $(GREEN)make run$(NC)          - Run Neo node (mainnet)"
	@echo "  $(GREEN)make run-testnet$(NC)  - Run Neo node (testnet)"
	@echo "  $(GREEN)make run-private$(NC)  - Run private network"
	@echo "  $(GREEN)make run-cli$(NC)      - Run Neo CLI"
	@echo "  $(GREEN)make run-rpc$(NC)      - Start RPC server"
	@echo "  $(GREEN)make mainnet$(NC)      - Run Neo node (mainnet) [alias for run]"
	@echo "  $(GREEN)make testnet$(NC)      - Run Neo node (testnet) [alias for run-testnet]"
	@echo ""
	@echo "$(BOLD)$(YELLOW)Docker:$(NC)"
	@echo "  $(GREEN)make docker$(NC)       - Build Docker image"
	@echo "  $(GREEN)make docker-run$(NC)   - Run in Docker"
	@echo "  $(GREEN)make docker-push$(NC)  - Push to registry"
	@echo "  $(GREEN)make run-docker-mainnet$(NC) - Run mainnet in Docker"
	@echo "  $(GREEN)make run-docker-testnet$(NC) - Run testnet in Docker"
	@echo ""
	@echo "$(BOLD)$(YELLOW)Development:$(NC)"
	@echo "  $(GREEN)make libs$(NC)         - Build core libraries only"
	@echo "  $(GREEN)make examples$(NC)     - Build and run examples"
	@echo "  $(GREEN)make bench$(NC)        - Run benchmarks"
	@echo "  $(GREEN)make docs$(NC)         - Generate documentation"
	@echo "  $(GREEN)make info$(NC)         - Show build configuration"
	@echo "  $(GREEN)make status$(NC)       - Show build status"
	@echo ""
	@echo "$(BOLD)$(YELLOW)Options:$(NC)"
	@echo "  $(BLUE)BUILD_TYPE$(NC)=$(BUILD_TYPE) (Debug|Release)"
	@echo "  $(BLUE)JOBS$(NC)=$(JOBS) (parallel jobs)"
	@echo "  $(BLUE)VERBOSE$(NC)=$(VERBOSE) (0|1)"
	@echo "  $(BLUE)ENABLE_TESTS$(NC)=$(ENABLE_TESTS)"
	@echo "  $(BLUE)ENABLE_ASAN$(NC)=$(ENABLE_ASAN) (AddressSanitizer)"
	@echo ""

# =============================================================================
# Build Targets
# =============================================================================

.PHONY: build
build: configure
	@echo "$(BOLD)$(CYAN)Building Neo C++ ($(BUILD_TYPE) mode)...$(NC)"
	@$(CMAKE) --build $(BUILD_DIR) -j $(JOBS) $(if $(filter 1,$(VERBOSE)),-v,)
	@echo "$(BOLD)$(GREEN)✓ Build completed successfully!$(NC)"
	@$(MAKE) -s status

.PHONY: configure
configure:
	@echo "$(CYAN)Configuring build...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
		-DNEO_BUILD_TESTS=$(ENABLE_TESTS) \
		-DNEO_BUILD_EXAMPLES=$(ENABLE_EXAMPLES) \
		-DNEO_BUILD_TOOLS=$(ENABLE_TOOLS) \
		-DNEO_BUILD_APPS=$(ENABLE_APPS) \
		-DENABLE_COVERAGE=$(ENABLE_COVERAGE) \
		-DENABLE_ASAN=$(ENABLE_ASAN) \
		-DENABLE_TSAN=$(ENABLE_TSAN)
	@echo "$(GREEN)✓ Configuration completed!$(NC)"

$(BUILD_DIR)/Makefile: configure

.PHONY: debug
debug:
	@$(MAKE) BUILD_TYPE=Debug build

.PHONY: release
release:
	@$(MAKE) BUILD_TYPE=Release build

.PHONY: libs
libs: configure
	@echo "$(CYAN)Building core libraries...$(NC)"
	@$(CMAKE) --build $(BUILD_DIR) --target neo_core -j $(JOBS)
	@$(CMAKE) --build $(BUILD_DIR) --target neo_io -j $(JOBS)
	@$(CMAKE) --build $(BUILD_DIR) --target neo_cryptography -j $(JOBS)
	@$(CMAKE) --build $(BUILD_DIR) --target neo_vm -j $(JOBS)
	@$(CMAKE) --build $(BUILD_DIR) --target neo_ledger -j $(JOBS)
	@$(CMAKE) --build $(BUILD_DIR) --target neo_network -j $(JOBS)
	@echo "$(GREEN)✓ Core libraries built!$(NC)"

# =============================================================================
# Testing Targets
# =============================================================================

.PHONY: test
test: build
	@echo "$(BOLD)$(CYAN)Running tests...$(NC)"
	@cd $(BUILD_DIR) && $(CTEST) --output-on-failure -j$(JOBS)
	@echo "$(BOLD)$(GREEN)✓ All tests passed!$(NC)"

.PHONY: test-unit
test-unit: build
	@echo "$(CYAN)Running unit tests...$(NC)"
	@cd $(BUILD_DIR) && $(CTEST) -R "^test_" --output-on-failure -j$(JOBS)

.PHONY: test-verbose
test-verbose: build
	@cd $(BUILD_DIR) && $(CTEST) -V -j$(JOBS)

.PHONY: coverage
coverage:
	@echo "$(CYAN)Generating coverage report...$(NC)"
	@$(MAKE) ENABLE_COVERAGE=ON BUILD_TYPE=Debug clean build test
	@echo "$(GREEN)✓ Coverage report generated$(NC)"

# =============================================================================
# Code Quality Targets
# =============================================================================

.PHONY: check
check: format tidy
	@echo "$(GREEN)✓ Code quality checks passed!$(NC)"

.PHONY: format
format:
	@echo "$(CYAN)Formatting code...$(NC)"
	@find src include -name "*.cpp" -o -name "*.h" | xargs $(CLANG_FORMAT) -i
	@echo "$(GREEN)✓ Code formatted!$(NC)"

.PHONY: format-check
format-check:
	@echo "$(CYAN)Checking code format...$(NC)"
	@find src include -name "*.cpp" -o -name "*.h" | xargs $(CLANG_FORMAT) --dry-run --Werror

.PHONY: tidy
tidy: configure
	@echo "$(CYAN)Running clang-tidy...$(NC)"
	@cd $(BUILD_DIR) && run-clang-tidy -p . -quiet || true
	@echo "$(GREEN)✓ Clang-tidy complete$(NC)"

# =============================================================================
# Running Applications
# =============================================================================

.PHONY: run
run: build
	@echo "$(BOLD)$(CYAN)Starting Neo node (MainNet)...$(NC)"
	@echo "$(YELLOW)Log level: info (use LOG_LEVEL env var to change)$(NC)"
	@$(BUILD_DIR)/apps/neo_node --network mainnet --config config/mainnet.config.json --log-level $${LOG_LEVEL:-info} --log-file neo-mainnet.log || \
	$(BUILD_DIR)/apps/neo_node --network mainnet --config config/mainnet.json --log-level $${LOG_LEVEL:-info} --log-file neo-mainnet.log || \
	$(BUILD_DIR)/apps/neo_node_production_ready --network mainnet --config config/mainnet.json --log-level $${LOG_LEVEL:-info} --log-file neo-mainnet.log || \
	$(BUILD_DIR)/apps/neo_node_app --network mainnet --config config/mainnet.json --log-level $${LOG_LEVEL:-info} --log-file neo-mainnet.log || \
	echo "$(RED)Neo node executable not found$(NC)"

.PHONY: run-testnet
run-testnet: build
	@echo "$(BOLD)$(CYAN)Starting Neo node (TestNet)...$(NC)"
	@echo "$(YELLOW)Log level: info (use LOG_LEVEL env var to change)$(NC)"
	@$(BUILD_DIR)/apps/neo_node --network testnet --config config/testnet.config.json --log-level $${LOG_LEVEL:-info} --log-file neo-testnet.log || \
	$(BUILD_DIR)/apps/neo_node --network testnet --config config/testnet.json --log-level $${LOG_LEVEL:-info} --log-file neo-testnet.log || \
	$(BUILD_DIR)/apps/neo_node_production_ready --network testnet --config config/testnet.json --log-level $${LOG_LEVEL:-info} --log-file neo-testnet.log || \
	$(BUILD_DIR)/apps/neo_node_app --network testnet --config config/testnet.json --log-level $${LOG_LEVEL:-info} --log-file neo-testnet.log || \
	echo "$(RED)Neo node executable not found$(NC)"

.PHONY: run-private
run-private: build
	@echo "$(BOLD)$(CYAN)Starting private network...$(NC)"
	@$(BUILD_DIR)/apps/neo_node --private --config config/private.json

.PHONY: run-cli
run-cli: build
	@echo "$(CYAN)Starting Neo CLI...$(NC)"
	@$(BUILD_DIR)/apps/cli/neo-cli --network testnet || $(BUILD_DIR)/apps/cli/neo-cli --config config/testnet.config.json || $(BUILD_DIR)/apps/cli/neo-cli --config config/testnet.json || true

.PHONY: run-rpc
run-rpc: build
	@echo "$(CYAN)Starting RPC server...$(NC)"
	@$(BUILD_DIR)/apps/neo_node --rpc --config config/rpc.json

# Aliases for requested naming convention
.PHONY: mainnet
mainnet: run

.PHONY: testnet
testnet: run-testnet

.PHONY: examples
examples: build
	@echo "$(CYAN)Running examples...$(NC)"
	@for example in $(BUILD_DIR)/examples/neo_example_*; do \
		if [ -f "$$example" ]; then \
			echo "$(BLUE)Running $$(basename $$example)...$(NC)"; \
			$$example || true; \
			echo ""; \
		fi; \
	done

.PHONY: bench
bench: build
	@echo "$(CYAN)Running benchmarks...$(NC)"
	@$(BUILD_DIR)/benchmarks/neo_benchmarks || echo "$(YELLOW)Benchmarks not built$(NC)"

# =============================================================================
# Docker Targets
# =============================================================================

.PHONY: docker
docker:
	@echo "$(BOLD)$(CYAN)Building Docker image $(DOCKER_IMAGE):$(DOCKER_TAG)...$(NC)"
	@if [ -x ./scripts/docker_build.sh ]; then \
		./scripts/docker_build.sh $(DOCKER_TAG) Dockerfile.optimized; \
	else \
		$(DOCKER) build \
			--build-arg VERSION=$(VERSION) \
			--build-arg BUILD_TYPE=$(BUILD_TYPE) \
			-t $(DOCKER_IMAGE):$(DOCKER_TAG) \
			-t $(DOCKER_IMAGE):latest \
			-f Dockerfile.optimized . || \
		$(DOCKER) build \
			--build-arg VERSION=$(VERSION) \
			--build-arg BUILD_TYPE=$(BUILD_TYPE) \
			-t $(DOCKER_IMAGE):$(DOCKER_TAG) \
			-t $(DOCKER_IMAGE):latest \
			-f Dockerfile .; \
	fi
	@echo "$(GREEN)✓ Docker image built successfully!$(NC)"

.PHONY: docker-run
docker-run: docker
	@echo "$(CYAN)Running Neo node in Docker...$(NC)"
	@$(DOCKER) run -it --rm \
		--name neo-node \
		-p 10332:10332 \
		-p 10333:10333 \
		-p 10334:10334 \
		-v neo-data:/data \
		-e NETWORK=mainnet \
		$(DOCKER_IMAGE):$(DOCKER_TAG)

.PHONY: docker-test
docker-test: docker
	@echo "$(CYAN)Running tests in Docker...$(NC)"
	@$(DOCKER) run --rm $(DOCKER_IMAGE):$(DOCKER_TAG) make test

.PHONY: docker-push
docker-push: docker
	@echo "$(CYAN)Pushing Docker image...$(NC)"
	@$(DOCKER) tag $(DOCKER_IMAGE):$(DOCKER_TAG) ghcr.io/r3e-network/$(DOCKER_IMAGE):$(DOCKER_TAG)
	@$(DOCKER) push ghcr.io/r3e-network/$(DOCKER_IMAGE):$(DOCKER_TAG)
	@echo "$(GREEN)✓ Image pushed to registry$(NC)"

.PHONY: run-docker-mainnet
run-docker-mainnet: docker
	@echo "$(BOLD)$(CYAN)Running Neo node (MainNet) in Docker...$(NC)"
	@$(DOCKER) run -it --rm \
		--name neo-node-mainnet \
		-p 10332:10332 \
		-p 10333:10333 \
		-p 10334:10334 \
		-v neo-mainnet-data:/data \
		-v $(PWD)/config:/config:ro \
		-e NETWORK=mainnet \
		-e LOG_LEVEL=info \
		$(DOCKER_IMAGE):$(DOCKER_TAG) \
		./neo_node --network mainnet --config /config/mainnet.json --log-level info

.PHONY: run-docker-testnet
run-docker-testnet: docker
	@echo "$(BOLD)$(CYAN)Running Neo node (TestNet) in Docker...$(NC)"
	@$(DOCKER) run -it --rm \
		--name neo-node-testnet \
		-p 20332:20332 \
		-p 20333:20333 \
		-p 20334:20334 \
		-v neo-testnet-data:/data \
		-v $(PWD)/config:/config:ro \
		-e NETWORK=testnet \
		-e LOG_LEVEL=info \
		$(DOCKER_IMAGE):$(DOCKER_TAG) \
		./neo_node --network testnet --config /config/testnet.json --log-level info

# =============================================================================
# Cleaning Targets
# =============================================================================

.PHONY: clean
clean:
	@echo "$(YELLOW)Cleaning build artifacts...$(NC)"
	@if [ -d $(BUILD_DIR) ]; then \
		$(CMAKE) --build $(BUILD_DIR) --target clean 2>/dev/null || true; \
	fi
	@echo "$(GREEN)✓ Clean completed!$(NC)"

.PHONY: distclean
distclean:
	@echo "$(YELLOW)Removing all build files...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -rf .cache
	@rm -f compile_commands.json
	@find . -name "*.o" -type f -delete 2>/dev/null || true
	@find . -name "*.a" -type f -delete 2>/dev/null || true
	@find . -name "*.so" -type f -delete 2>/dev/null || true
	@find . -name "*.dylib" -type f -delete 2>/dev/null || true
	@echo "$(GREEN)✓ Distclean completed!$(NC)"

# =============================================================================
# Ccache Targets
# =============================================================================

.PHONY: ccache-stats
ccache-stats:
	@echo "$(BOLD)$(CYAN)Ccache Statistics$(NC)"
	@echo "$(WHITE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@ccache -s || echo "$(YELLOW)ccache not available$(NC)"

.PHONY: ccache-clean
ccache-clean:
	@echo "$(YELLOW)Clearing ccache...$(NC)"
	@ccache -C || echo "$(YELLOW)ccache not available$(NC)"
	@echo "$(GREEN)✓ Ccache cleared!$(NC)"

# =============================================================================
# Installation Targets
# =============================================================================

.PHONY: install
install: build
	@echo "$(CYAN)Installing Neo C++...$(NC)"
	@$(CMAKE) --install $(BUILD_DIR)
	@echo "$(GREEN)✓ Neo C++ installed to $(INSTALL_PREFIX)$(NC)"

.PHONY: uninstall
uninstall:
	@echo "$(CYAN)Uninstalling Neo C++...$(NC)"
	@if [ -f $(BUILD_DIR)/install_manifest.txt ]; then \
		xargs rm -f < $(BUILD_DIR)/install_manifest.txt; \
		echo "$(GREEN)✓ Neo C++ uninstalled$(NC)"; \
	else \
		echo "$(YELLOW)No installation manifest found$(NC)"; \
	fi

# =============================================================================
# Documentation Targets
# =============================================================================

.PHONY: docs
docs:
	@echo "$(CYAN)Generating documentation...$(NC)"
	@doxygen Doxyfile || echo "$(YELLOW)Doxygen not found. Install with: brew install doxygen$(NC)"
	@echo "$(GREEN)✓ Documentation generated$(NC)"

# =============================================================================
# Information Targets
# =============================================================================

.PHONY: info
info:
	@echo "$(BOLD)$(CYAN)Neo C++ Build Configuration$(NC)"
	@echo "$(WHITE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "  $(YELLOW)Version:$(NC)        $(VERSION)"
	@echo "  $(YELLOW)Commit:$(NC)         $(COMMIT)"
	@echo "  $(YELLOW)Build Date:$(NC)     $(BUILD_DATE)"
	@echo "  $(YELLOW)Platform:$(NC)       $(PLATFORM)"
	@echo "  $(YELLOW)Build Type:$(NC)     $(BUILD_TYPE)"
	@echo "  $(YELLOW)Jobs:$(NC)           $(JOBS)"
	@echo "  $(YELLOW)Install Prefix:$(NC) $(INSTALL_PREFIX)"
	@echo ""
	@echo "$(CYAN)Features:$(NC)"
	@echo "  $(YELLOW)Tests:$(NC)          $(ENABLE_TESTS)"
	@echo "  $(YELLOW)Examples:$(NC)       $(ENABLE_EXAMPLES)"
	@echo "  $(YELLOW)Tools:$(NC)          $(ENABLE_TOOLS)"
	@echo "  $(YELLOW)Apps:$(NC)           $(ENABLE_APPS)"

.PHONY: status
status:
	@echo "$(CYAN)Build Status:$(NC)"
	@if [ -d $(BUILD_DIR) ]; then \
		echo "  $(GREEN)✓$(NC) Build directory exists"; \
		if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
			echo "  $(GREEN)✓$(NC) CMake configured"; \
		fi; \
		echo ""; \
		echo "$(CYAN)Components Built:$(NC)"; \
		for lib in core io cryptography vm ledger network consensus smartcontract rpc plugins; do \
			if find $(BUILD_DIR) -name "*neo_$$lib*" 2>/dev/null | grep -q .; then \
				echo "  $(GREEN)✓$(NC) neo_$$lib"; \
			else \
				echo "  $(YELLOW)○$(NC) neo_$$lib"; \
			fi; \
		done; \
	else \
		echo "  $(RED)✗$(NC) Build directory not found. Run 'make' first."; \
	fi

.PHONY: version
version:
	@echo "$(VERSION)"

# =============================================================================
# Development Utilities
# =============================================================================

.PHONY: deps
deps:
	@echo "$(CYAN)Installing dependencies...$(NC)"
	@if [ "$(PLATFORM)" = "macos" ]; then \
		brew install cmake openssl boost rocksdb nlohmann-json spdlog gtest clang-format; \
	elif [ "$(PLATFORM)" = "linux" ]; then \
		sudo apt-get update && sudo apt-get install -y \
			cmake build-essential libssl-dev libboost-all-dev \
			librocksdb-dev nlohmann-json3-dev libspdlog-dev \
			libgtest-dev clang-format clang-tidy; \
	fi
	@echo "$(GREEN)✓ Dependencies installed$(NC)"

.PHONY: ci
ci:
	@echo "$(BOLD)$(CYAN)Running CI pipeline...$(NC)"
	@$(MAKE) clean
	@$(MAKE) BUILD_TYPE=Debug ENABLE_TESTS=ON build
	@$(MAKE) test
	@$(MAKE) format-check
	@echo "$(BOLD)$(GREEN)✓ CI pipeline passed!$(NC)"

# =============================================================================
# Shortcuts
# =============================================================================

.PHONY: b
b: build

.PHONY: t
t: test

.PHONY: r
r: run

.PHONY: c
c: clean

.PHONY: h
h: help

# =============================================================================
# Smoke Tests (offline initialization)
# =============================================================================

.PHONY: smoke-node
smoke-node:
	@echo "$(CYAN)Running offline smoke test with $(SMOKE_CONFIG)$(NC)"
	cd $(BUILD_DIR) && ./apps/neo_node --config ../$${SMOKE_CONFIG:-config/testnet.config.json} --no-rpc --status-interval 2 || true

.PHONY: smoke-cli
smoke-cli:
	@echo "$(CYAN)Running offline CLI smoke test with $(SMOKE_CONFIG)$(NC)"
	cd $(BUILD_DIR) && ./apps/cli/neo-cli --config ../$${SMOKE_CONFIG:-config/testnet.config.json} --noverify --no-rpc --status-interval 2 || true

# Force rebuild
.PHONY: rebuild
rebuild: clean build

.PHONY: .FORCE
.FORCE:
