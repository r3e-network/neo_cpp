# Docker.cmake - Docker integration for Neo C++

# Check if Docker is available
find_program(DOCKER_EXECUTABLE docker)

if(NOT DOCKER_EXECUTABLE)
    message(WARNING "Docker not found. Docker targets will not be available.")
    return()
endif()

# Docker image settings
set(DOCKER_IMAGE_NAME "neo-cpp" CACHE STRING "Docker image name")
set(DOCKER_IMAGE_TAG "latest" CACHE STRING "Docker image tag")
set(DOCKER_REGISTRY "" CACHE STRING "Docker registry (optional)")

# Full image name
if(DOCKER_REGISTRY)
    set(DOCKER_IMAGE "${DOCKER_REGISTRY}/${DOCKER_IMAGE_NAME}:${DOCKER_IMAGE_TAG}")
else()
    set(DOCKER_IMAGE "${DOCKER_IMAGE_NAME}:${DOCKER_IMAGE_TAG}")
endif()

# Docker build arguments
set(DOCKER_BUILD_ARGS "" CACHE STRING "Additional docker build arguments")

# =============================================================================
# Docker Build Targets
# =============================================================================

# Build Docker image
add_custom_target(docker
    COMMAND ${DOCKER_EXECUTABLE} build 
        -t ${DOCKER_IMAGE}
        -f ${CMAKE_SOURCE_DIR}/Dockerfile
        ${DOCKER_BUILD_ARGS}
        ${CMAKE_SOURCE_DIR}
    COMMENT "Building Docker image: ${DOCKER_IMAGE}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

# Build Docker image with no cache
add_custom_target(docker-nocache
    COMMAND ${DOCKER_EXECUTABLE} build 
        --no-cache
        -t ${DOCKER_IMAGE}
        -f ${CMAKE_SOURCE_DIR}/Dockerfile
        ${DOCKER_BUILD_ARGS}
        ${CMAKE_SOURCE_DIR}
    COMMENT "Building Docker image (no cache): ${DOCKER_IMAGE}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

# Build multi-platform Docker image
add_custom_target(docker-multiplatform
    COMMAND ${DOCKER_EXECUTABLE} buildx build 
        --platform linux/amd64,linux/arm64
        -t ${DOCKER_IMAGE}
        -f ${CMAKE_SOURCE_DIR}/Dockerfile
        ${DOCKER_BUILD_ARGS}
        ${CMAKE_SOURCE_DIR}
    COMMENT "Building multi-platform Docker image: ${DOCKER_IMAGE}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

# =============================================================================
# Docker Run Targets
# =============================================================================

# Run Docker container (default network)
add_custom_target(docker-run
    COMMAND ${DOCKER_EXECUTABLE} run 
        --rm 
        -it 
        --name neo-cpp-node
        -p 10332:10332 
        -p 10333:10333
        -v neo-data:/var/lib/neo
        -v neo-logs:/var/log/neo
        ${DOCKER_IMAGE}
    COMMENT "Running Docker container: ${DOCKER_IMAGE}"
    VERBATIM
)

# Run Docker container on MainNet
add_custom_target(docker-run-mainnet
    COMMAND ${DOCKER_EXECUTABLE} run 
        --rm 
        -it 
        --name neo-cpp-mainnet
        -p 10332:10332 
        -p 10333:10333
        -v neo-mainnet-data:/var/lib/neo
        -v neo-mainnet-logs:/var/log/neo
        ${DOCKER_IMAGE} mainnet
    COMMENT "Running Docker container on MainNet"
    VERBATIM
)

# Run Docker container on TestNet
add_custom_target(docker-run-testnet
    COMMAND ${DOCKER_EXECUTABLE} run 
        --rm 
        -it 
        --name neo-cpp-testnet
        -p 20332:10332 
        -p 20333:10333
        -v neo-testnet-data:/var/lib/neo
        -v neo-testnet-logs:/var/log/neo
        ${DOCKER_IMAGE} testnet
    COMMENT "Running Docker container on TestNet"
    VERBATIM
)

# Run Docker container on Private network
add_custom_target(docker-run-private
    COMMAND ${DOCKER_EXECUTABLE} run 
        --rm 
        -it 
        --name neo-cpp-private
        -p 30332:10332 
        -p 30333:10333
        -v neo-private-data:/var/lib/neo
        -v neo-private-logs:/var/log/neo
        -v ${CMAKE_SOURCE_DIR}/config/private.json:/etc/neo/private.json:ro
        ${DOCKER_IMAGE} private
    COMMENT "Running Docker container on Private network"
    VERBATIM
)

# Run Docker container in detached mode
add_custom_target(docker-run-detached
    COMMAND ${DOCKER_EXECUTABLE} run 
        -d 
        --name neo-cpp-node
        --restart unless-stopped
        -p 10332:10332 
        -p 10333:10333
        -v neo-data:/var/lib/neo
        -v neo-logs:/var/log/neo
        ${DOCKER_IMAGE}
    COMMENT "Running Docker container in detached mode"
    VERBATIM
)

# =============================================================================
# Docker Management Targets
# =============================================================================

# Stop Docker container
add_custom_target(docker-stop
    COMMAND ${DOCKER_EXECUTABLE} stop neo-cpp-node || true
    COMMAND ${DOCKER_EXECUTABLE} stop neo-cpp-mainnet || true
    COMMAND ${DOCKER_EXECUTABLE} stop neo-cpp-testnet || true
    COMMAND ${DOCKER_EXECUTABLE} stop neo-cpp-private || true
    COMMENT "Stopping Docker containers"
    VERBATIM
)

# Remove Docker container
add_custom_target(docker-rm
    COMMAND ${DOCKER_EXECUTABLE} rm -f neo-cpp-node || true
    COMMAND ${DOCKER_EXECUTABLE} rm -f neo-cpp-mainnet || true
    COMMAND ${DOCKER_EXECUTABLE} rm -f neo-cpp-testnet || true
    COMMAND ${DOCKER_EXECUTABLE} rm -f neo-cpp-private || true
    COMMENT "Removing Docker containers"
    VERBATIM
)

# View Docker logs
add_custom_target(docker-logs
    COMMAND ${DOCKER_EXECUTABLE} logs -f neo-cpp-node || 
            ${DOCKER_EXECUTABLE} logs -f neo-cpp-mainnet ||
            ${DOCKER_EXECUTABLE} logs -f neo-cpp-testnet ||
            ${DOCKER_EXECUTABLE} logs -f neo-cpp-private
    COMMENT "Viewing Docker container logs"
    VERBATIM
)

# Execute shell in Docker container
add_custom_target(docker-shell
    COMMAND ${DOCKER_EXECUTABLE} exec -it neo-cpp-node /bin/bash ||
            ${DOCKER_EXECUTABLE} exec -it neo-cpp-mainnet /bin/bash ||
            ${DOCKER_EXECUTABLE} exec -it neo-cpp-testnet /bin/bash ||
            ${DOCKER_EXECUTABLE} exec -it neo-cpp-private /bin/bash
    COMMENT "Opening shell in Docker container"
    VERBATIM
)

# Docker container status
add_custom_target(docker-ps
    COMMAND ${DOCKER_EXECUTABLE} ps -a | grep neo-cpp || echo "No Neo C++ containers found"
    COMMENT "Showing Docker container status"
    VERBATIM
)

# Clean Docker volumes
add_custom_target(docker-clean-volumes
    COMMAND ${DOCKER_EXECUTABLE} volume rm neo-data neo-logs || true
    COMMAND ${DOCKER_EXECUTABLE} volume rm neo-mainnet-data neo-mainnet-logs || true
    COMMAND ${DOCKER_EXECUTABLE} volume rm neo-testnet-data neo-testnet-logs || true
    COMMAND ${DOCKER_EXECUTABLE} volume rm neo-private-data neo-private-logs || true
    COMMENT "Cleaning Docker volumes"
    VERBATIM
)

# =============================================================================
# Docker Compose Targets
# =============================================================================

# Check for docker-compose
find_program(DOCKER_COMPOSE_EXECUTABLE docker-compose)
if(NOT DOCKER_COMPOSE_EXECUTABLE)
    # Try docker compose (newer syntax)
    execute_process(
        COMMAND ${DOCKER_EXECUTABLE} compose version
        RESULT_VARIABLE DOCKER_COMPOSE_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(DOCKER_COMPOSE_RESULT EQUAL 0)
        set(DOCKER_COMPOSE_CMD "${DOCKER_EXECUTABLE} compose")
    endif()
else()
    set(DOCKER_COMPOSE_CMD "${DOCKER_COMPOSE_EXECUTABLE}")
endif()

if(DOCKER_COMPOSE_CMD)
    # Docker Compose up
    add_custom_target(docker-compose-up
        COMMAND ${DOCKER_COMPOSE_CMD} up -d
        COMMENT "Starting services with Docker Compose"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
    )

    # Docker Compose down
    add_custom_target(docker-compose-down
        COMMAND ${DOCKER_COMPOSE_CMD} down
        COMMENT "Stopping services with Docker Compose"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
    )

    # Docker Compose logs
    add_custom_target(docker-compose-logs
        COMMAND ${DOCKER_COMPOSE_CMD} logs -f
        COMMENT "Viewing Docker Compose logs"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
    )

    # Docker Compose restart
    add_custom_target(docker-compose-restart
        COMMAND ${DOCKER_COMPOSE_CMD} restart
        COMMENT "Restarting services with Docker Compose"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
    )
endif()

# =============================================================================
# Docker Registry Targets
# =============================================================================

# Push Docker image to registry
add_custom_target(docker-push
    COMMAND ${DOCKER_EXECUTABLE} push ${DOCKER_IMAGE}
    COMMENT "Pushing Docker image to registry: ${DOCKER_IMAGE}"
    VERBATIM
)

# Pull Docker image from registry
add_custom_target(docker-pull
    COMMAND ${DOCKER_EXECUTABLE} pull ${DOCKER_IMAGE}
    COMMENT "Pulling Docker image from registry: ${DOCKER_IMAGE}"
    VERBATIM
)

# Tag Docker image
add_custom_target(docker-tag
    COMMAND ${DOCKER_EXECUTABLE} tag ${DOCKER_IMAGE} ${DOCKER_IMAGE_NAME}:${GIT_COMMIT_HASH}
    COMMAND ${DOCKER_EXECUTABLE} tag ${DOCKER_IMAGE} ${DOCKER_IMAGE_NAME}:latest
    COMMENT "Tagging Docker image"
    VERBATIM
)

# =============================================================================
# Docker Development Targets
# =============================================================================

# Build and run Docker image
add_custom_target(docker-dev
    COMMAND ${CMAKE_COMMAND} --build . --target docker
    COMMAND ${CMAKE_COMMAND} --build . --target docker-run
    COMMENT "Building and running Docker image for development"
    VERBATIM
)

# Rebuild and run Docker image
add_custom_target(docker-rebuild
    COMMAND ${CMAKE_COMMAND} --build . --target docker-nocache
    COMMAND ${CMAKE_COMMAND} --build . --target docker-run
    COMMENT "Rebuilding and running Docker image"
    VERBATIM
)

# =============================================================================
# Help Target
# =============================================================================

add_custom_target(docker-help
    COMMAND ${CMAKE_COMMAND} -E echo "Docker targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker                 - Build Docker image"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-nocache         - Build Docker image without cache"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-multiplatform   - Build multi-platform Docker image"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-run             - Run Docker container (interactive)"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-run-mainnet     - Run on MainNet"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-run-testnet     - Run on TestNet"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-run-private     - Run on Private network"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-run-detached    - Run in background"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-stop            - Stop containers"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-rm              - Remove containers"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-logs            - View logs"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-shell           - Open shell in container"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-ps              - Show container status"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-clean-volumes   - Clean volumes"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-compose-up      - Start with docker-compose"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-compose-down    - Stop with docker-compose"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-compose-logs    - View compose logs"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-push            - Push to registry"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-pull            - Pull from registry"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-tag             - Tag image"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-dev             - Build and run for development"
    COMMAND ${CMAKE_COMMAND} -E echo "  docker-rebuild         - Rebuild and run"
    COMMENT "Showing Docker targets help"
    VERBATIM
)

message(STATUS "Docker support enabled. Run 'make docker-help' for available targets.")