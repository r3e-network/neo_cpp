# CMake Docker Integration Reference

## Overview

This document details the Docker integration in the Neo C++ CMake build system, providing developers with a complete reference for all Docker-related targets and their usage.

## Integration Architecture

```
CMakeLists.txt
    ├── cmake/Docker.cmake      # Docker target definitions
    ├── Dockerfile              # Multi-stage build configuration
    ├── docker-entrypoint.sh    # Smart entrypoint script
    └── docker-compose.yml      # Multi-container orchestration
```

## Available Make Targets

### Complete Target List

| Target | Description | Usage |
|--------|-------------|-------|
| `docker` | Build Docker image | `make docker` |
| `docker-nocache` | Build without cache | `make docker-nocache` |
| `docker-multiplatform` | Build for multiple architectures | `make docker-multiplatform` |
| `docker-run` | Run container interactively | `make docker-run` |
| `docker-run-mainnet` | Run on MainNet | `make docker-run-mainnet` |
| `docker-run-testnet` | Run on TestNet | `make docker-run-testnet` |
| `docker-run-private` | Run on private network | `make docker-run-private` |
| `docker-run-detached` | Run in background | `make docker-run-detached` |
| `docker-stop` | Stop all containers | `make docker-stop` |
| `docker-rm` | Remove containers | `make docker-rm` |
| `docker-logs` | View container logs | `make docker-logs` |
| `docker-shell` | Open shell in container | `make docker-shell` |
| `docker-ps` | Show container status | `make docker-ps` |
| `docker-clean-volumes` | Clean Docker volumes | `make docker-clean-volumes` |
| `docker-compose-up` | Start with docker-compose | `make docker-compose-up` |
| `docker-compose-down` | Stop docker-compose | `make docker-compose-down` |
| `docker-compose-logs` | View compose logs | `make docker-compose-logs` |
| `docker-compose-restart` | Restart services | `make docker-compose-restart` |
| `docker-push` | Push to registry | `make docker-push` |
| `docker-pull` | Pull from registry | `make docker-pull` |
| `docker-tag` | Tag image | `make docker-tag` |
| `docker-dev` | Build and run for development | `make docker-dev` |
| `docker-rebuild` | Rebuild and run | `make docker-rebuild` |
| `docker-help` | Show Docker help | `make docker-help` |

## CMake Configuration

### Basic Setup

```bash
mkdir build && cd build
cmake ..
```

### Custom Docker Configuration

```bash
cmake .. \
    -DDOCKER_IMAGE_NAME=my-neo-node \
    -DDOCKER_IMAGE_TAG=v1.0.0 \
    -DDOCKER_REGISTRY=myregistry.com \
    -DDOCKER_BUILD_ARGS="--platform=linux/amd64"
```

### CMake Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `DOCKER_IMAGE_NAME` | Docker image name | `neo-cpp` |
| `DOCKER_IMAGE_TAG` | Docker image tag | `latest` |
| `DOCKER_REGISTRY` | Docker registry URL | (empty) |
| `DOCKER_BUILD_ARGS` | Additional build arguments | (empty) |

## Target Details

### Build Targets

#### `make docker`

Builds the Docker image with default settings:

```bash
docker build \
    -t neo-cpp:latest \
    -f Dockerfile \
    .
```

#### `make docker-nocache`

Forces a complete rebuild without using cache:

```bash
docker build \
    --no-cache \
    -t neo-cpp:latest \
    -f Dockerfile \
    .
```

#### `make docker-multiplatform`

Builds for multiple platforms using buildx:

```bash
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    -t neo-cpp:latest \
    -f Dockerfile \
    .
```

### Run Targets

#### `make docker-run-mainnet`

Runs container configured for MainNet:

```bash
docker run \
    --rm \
    -it \
    --name neo-cpp-mainnet \
    -p 10332:10332 \
    -p 10333:10333 \
    -v neo-mainnet-data:/var/lib/neo \
    -v neo-mainnet-logs:/var/log/neo \
    neo-cpp:latest mainnet
```

#### `make docker-run-testnet`

Runs container configured for TestNet:

```bash
docker run \
    --rm \
    -it \
    --name neo-cpp-testnet \
    -p 20332:10332 \
    -p 20333:10333 \
    -v neo-testnet-data:/var/lib/neo \
    -v neo-testnet-logs:/var/log/neo \
    neo-cpp:latest testnet
```

### Management Targets

#### `make docker-logs`

Follows logs from running container:

```bash
docker logs -f neo-cpp-node || \
docker logs -f neo-cpp-mainnet || \
docker logs -f neo-cpp-testnet
```

#### `make docker-shell`

Opens interactive shell in running container:

```bash
docker exec -it neo-cpp-node /bin/bash
```

## Docker Compose Integration

### Compose Targets

The CMake system automatically detects docker-compose availability and provides targets:

```cmake
find_program(DOCKER_COMPOSE_EXECUTABLE docker-compose)
if(NOT DOCKER_COMPOSE_EXECUTABLE)
    execute_process(
        COMMAND ${DOCKER_EXECUTABLE} compose version
        RESULT_VARIABLE DOCKER_COMPOSE_RESULT
    )
endif()
```

### Service Management

```bash
# Start all services
make docker-compose-up

# Stop all services
make docker-compose-down

# View logs
make docker-compose-logs

# Restart services
make docker-compose-restart
```

## Volume Management

### Named Volumes

The system uses named volumes for persistent storage:

| Volume | Purpose | Network |
|--------|---------|---------|
| `neo-mainnet-data` | MainNet blockchain data | MainNet |
| `neo-mainnet-logs` | MainNet logs | MainNet |
| `neo-testnet-data` | TestNet blockchain data | TestNet |
| `neo-testnet-logs` | TestNet logs | TestNet |
| `neo-private-data` | Private network data | Private |
| `neo-private-logs` | Private network logs | Private |

### Volume Operations

```bash
# List volumes
docker volume ls | grep neo

# Inspect volume
docker volume inspect neo-mainnet-data

# Clean all volumes
make docker-clean-volumes
```

## Network Configuration

### Port Mappings

The CMake targets automatically configure ports based on network:

```cmake
# MainNet
-p 10332:10332  # RPC
-p 10333:10333  # P2P

# TestNet
-p 20332:10332  # RPC (mapped to 20332)
-p 20333:10333  # P2P (mapped to 20333)

# Private
-p 30332:10332  # RPC (mapped to 30332)
-p 30333:10333  # P2P (mapped to 30333)
```

## Development Workflows

### Quick Development Cycle

```bash
# 1. Make code changes
vim src/somefile.cpp

# 2. Build and run in Docker
make docker-dev

# 3. Check logs
make docker-logs

# 4. Debug if needed
make docker-shell
```

### Testing in Docker

```bash
# Build image
make docker

# Run tests in container
docker run --rm neo-cpp:latest ctest

# Or use custom test command
docker run --rm neo-cpp:latest ./run_tests.sh
```

### Continuous Integration

```yaml
# .github/workflows/docker.yml
name: Docker Build
on: [push, pull_request]
jobs:
  docker:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Configure CMake
      run: |
        mkdir build
        cd build
        cmake ..
    - name: Build Docker Image
      run: |
        cd build
        make docker
    - name: Test Docker Image
      run: |
        cd build
        docker run --rm neo-cpp:latest neo_cli_tool version
```

## Registry Integration

### Push to Docker Hub

```bash
# Configure registry
cmake .. -DDOCKER_REGISTRY=docker.io/yourusername

# Build and push
make docker
make docker-push
```

### Private Registry

```bash
# Configure private registry
cmake .. -DDOCKER_REGISTRY=registry.company.com

# Login to registry
docker login registry.company.com

# Build and push
make docker
make docker-push
```

## Troubleshooting

### Common Issues

#### Docker Not Found

```cmake
find_program(DOCKER_EXECUTABLE docker)
if(NOT DOCKER_EXECUTABLE)
    message(WARNING "Docker not found. Docker targets will not be available.")
    return()
endif()
```

#### Permission Denied

```bash
# Add user to docker group
sudo usermod -aG docker $USER

# Restart session
newgrp docker
```

#### Port Already in Use

```bash
# Find process using port
lsof -i :10332

# Stop conflicting container
docker stop $(docker ps -q --filter "publish=10332")
```

### Debug CMake Docker Integration

```bash
# Verbose CMake output
cmake .. --debug-output

# Check generated targets
make help | grep docker

# Test individual components
cmake --build . --target docker-help
```

## Advanced Configuration

### Custom Dockerfile

Modify `cmake/Docker.cmake` to use custom Dockerfile:

```cmake
set(DOCKER_FILE "${CMAKE_SOURCE_DIR}/Dockerfile.custom" 
    CACHE STRING "Dockerfile to use")

add_custom_target(docker
    COMMAND ${DOCKER_EXECUTABLE} build 
        -f ${DOCKER_FILE}
        ...
)
```

### Build Arguments

Pass build arguments through CMake:

```cmake
set(DOCKER_BUILD_ARGS 
    "--build-arg VERSION=${PROJECT_VERSION}"
    "--build-arg BUILD_DATE=$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
    CACHE STRING "Docker build arguments")
```

### Multi-Stage Build Options

Control multi-stage build targets:

```cmake
# Build only builder stage
add_custom_target(docker-builder
    COMMAND ${DOCKER_EXECUTABLE} build 
        --target builder
        -t neo-cpp:builder
        .
)
```

## Best Practices

### 1. Version Tagging

Always tag images with version:

```cmake
add_custom_target(docker-version
    COMMAND ${DOCKER_EXECUTABLE} build 
        -t neo-cpp:${PROJECT_VERSION}
        -t neo-cpp:latest
        .
)
```

### 2. Cache Optimization

Use Docker build cache effectively:

```dockerfile
# Copy only dependency files first
COPY CMakeLists.txt package.json ./
RUN npm install

# Then copy source code
COPY src/ ./src/
```

### 3. Security

Run containers with minimal privileges:

```cmake
add_custom_target(docker-run-secure
    COMMAND ${DOCKER_EXECUTABLE} run 
        --read-only
        --security-opt=no-new-privileges
        --cap-drop=ALL
        neo-cpp:latest
)
```

## Integration with Other Tools

### Integration with CTest

```cmake
add_test(NAME docker_test
    COMMAND ${DOCKER_EXECUTABLE} run --rm neo-cpp:latest ctest
)
```

### Integration with CPack

```cmake
# Package Docker image
install(CODE "
    execute_process(
        COMMAND docker save neo-cpp:latest -o neo-cpp.tar
    )
")
```

## Contributing

To add new Docker targets:

1. Edit `cmake/Docker.cmake`
2. Follow the existing pattern:
   ```cmake
   add_custom_target(docker-new-target
       COMMAND ${DOCKER_EXECUTABLE} ...
       COMMENT "Description..."
       VERBATIM
   )
   ```
3. Update help target
4. Test the new target
5. Document in this guide

## References

- [CMake Documentation](https://cmake.org/documentation/)
- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Documentation](https://docs.docker.com/compose/)
- [Neo C++ Docker Guide](./DOCKER_GUIDE.md)

---

*Last Updated: August 15, 2025*  
*CMake Minimum Version: 3.20*  
*Docker Minimum Version: 20.10*