# Neo C++ Docker Guide

## Overview

This guide provides comprehensive documentation for building, running, and managing Neo C++ nodes using Docker. The project includes full Docker and Docker Compose integration directly in the CMake build system.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [Building Docker Images](#building-docker-images)
4. [Running Containers](#running-containers)
5. [Network Configuration](#network-configuration)
6. [Docker Compose](#docker-compose)
7. [Development Workflow](#development-workflow)
8. [Production Deployment](#production-deployment)
9. [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Software
- Docker 20.10+ or Docker Desktop
- Docker Compose 2.0+ (optional, for multi-container setups)
- CMake 3.20+ (for make targets)
- Git

### System Requirements
- **Minimum**: 2 CPU cores, 4GB RAM, 20GB disk space
- **Recommended**: 4 CPU cores, 8GB RAM, 100GB disk space
- **Production**: 8+ CPU cores, 16GB+ RAM, 500GB+ SSD

## Quick Start

### 1. Clone and Configure

```bash
git clone https://github.com/r3e-network/neo_cpp.git
cd neo_cpp
mkdir build && cd build
cmake ..
```

### 2. Build Docker Image

```bash
make docker
```

### 3. Run Neo Node

```bash
# Run on MainNet
make docker-run-mainnet

# Run on TestNet
make docker-run-testnet

# Run on Private Network
make docker-run-private
```

## Building Docker Images

### Basic Build

Build the default Docker image:

```bash
make docker
```

This creates an image tagged as `neo-cpp:latest`.

### Build Without Cache

Force a complete rebuild:

```bash
make docker-nocache
```

### Multi-Platform Build

Build for multiple architectures (AMD64 and ARM64):

```bash
make docker-multiplatform
```

### Custom Build Arguments

You can customize the build with CMake variables:

```bash
cmake .. -DDOCKER_IMAGE_NAME=my-neo-node \
         -DDOCKER_IMAGE_TAG=v1.0.0 \
         -DDOCKER_REGISTRY=myregistry.com

make docker
```

## Running Containers

### Interactive Mode (Default)

Run a container interactively with automatic cleanup:

```bash
make docker-run
```

### Network-Specific Commands

#### MainNet

```bash
make docker-run-mainnet
```

- **Ports**: 10332 (RPC), 10333 (P2P)
- **Volumes**: `neo-mainnet-data`, `neo-mainnet-logs`
- **Config**: Uses `config/mainnet.json`

#### TestNet

```bash
make docker-run-testnet
```

- **Ports**: 20332 (RPC), 20333 (P2P)
- **Volumes**: `neo-testnet-data`, `neo-testnet-logs`
- **Config**: Uses `config/testnet.json`

#### Private Network

```bash
make docker-run-private
```

- **Ports**: 30332 (RPC), 30333 (P2P)
- **Volumes**: `neo-private-data`, `neo-private-logs`
- **Config**: Uses `config/private.json`

### Detached Mode

Run in the background:

```bash
make docker-run-detached
```

### Container Management

#### View Status

```bash
make docker-ps
```

#### View Logs

```bash
make docker-logs
```

#### Access Shell

```bash
make docker-shell
```

#### Stop Containers

```bash
make docker-stop
```

#### Remove Containers

```bash
make docker-rm
```

#### Clean Volumes

```bash
make docker-clean-volumes
```

## Network Configuration

### Environment Variables

The Docker container supports the following environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `NEO_NETWORK` | Network type (mainnet/testnet/private) | mainnet |
| `NEO_CONFIG_FILE` | Configuration file path | /etc/neo/config.json |
| `NEO_DATA_PATH` | Blockchain data directory | /var/lib/neo |
| `NEO_LOG_PATH` | Log file directory | /var/log/neo |
| `NEO_LOG_LEVEL` | Logging level (DEBUG/INFO/WARN/ERROR) | INFO |

### Custom Configuration

Mount your own configuration:

```bash
docker run -v /path/to/config.json:/etc/neo/config.json:ro neo-cpp:latest
```

### Port Mapping

Standard port mappings by network:

| Network | RPC Port | P2P Port | WebSocket |
|---------|----------|----------|-----------|
| MainNet | 10332 | 10333 | 10334 |
| TestNet | 20332 | 20333 | 20334 |
| Private | 30332 | 30333 | 30334 |

## Docker Compose

### Starting Services

Start all services defined in `docker-compose.yml`:

```bash
make docker-compose-up
```

### Service Architecture

The docker-compose.yml includes:

- **neo-mainnet**: MainNet node
- **neo-testnet**: TestNet node  
- **neo-privatenet**: Private network node
- **prometheus**: Metrics collection
- **grafana**: Metrics visualization

### Managing Services

```bash
# Stop all services
make docker-compose-down

# View logs
make docker-compose-logs

# Restart services
make docker-compose-restart
```

### Custom Compose File

Use a custom docker-compose file:

```bash
docker compose -f docker-compose.custom.yml up
```

## Development Workflow

### Rapid Development Cycle

Build and run in one command:

```bash
make docker-dev
```

### Rebuild and Run

Force rebuild and run:

```bash
make docker-rebuild
```

### Development Best Practices

1. **Use Volume Mounts** for configuration:
   ```bash
   docker run -v $(pwd)/config:/etc/neo:ro neo-cpp:latest
   ```

2. **Enable Debug Logging**:
   ```bash
   docker run -e NEO_LOG_LEVEL=DEBUG neo-cpp:latest
   ```

3. **Mount Source Code** for live development:
   ```bash
   docker run -v $(pwd)/src:/app/src neo-cpp:latest
   ```

### Debugging

Access container for debugging:

```bash
# Start container
make docker-run-testnet

# In another terminal
make docker-shell

# Inside container
neo_cli_tool status
tail -f /var/log/neo/node.log
```

## Production Deployment

### Security Considerations

1. **Non-Root User**: Container runs as `neo` user (UID 1000)
2. **Read-Only Root**: Use `--read-only` flag
3. **Security Options**:
   ```bash
   docker run --security-opt=no-new-privileges \
              --cap-drop=ALL \
              --cap-add=NET_BIND_SERVICE \
              neo-cpp:latest
   ```

### Resource Limits

Set resource constraints:

```bash
docker run --memory="4g" \
           --memory-swap="4g" \
           --cpus="2.0" \
           --pids-limit=200 \
           neo-cpp:latest
```

### Health Monitoring

The container includes health checks:

```dockerfile
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD neo-cli rpc getblockcount || exit 1
```

Monitor health status:

```bash
docker ps --format "table {{.Names}}\t{{.Status}}"
```

### Backup and Recovery

#### Backup Data

```bash
# Create backup
docker run --rm -v neo-mainnet-data:/data \
           -v $(pwd)/backup:/backup \
           alpine tar czf /backup/neo-backup.tar.gz /data
```

#### Restore Data

```bash
# Restore backup
docker run --rm -v neo-mainnet-data:/data \
           -v $(pwd)/backup:/backup \
           alpine tar xzf /backup/neo-backup.tar.gz -C /
```

### Kubernetes Deployment

Example Kubernetes deployment:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: neo-node
spec:
  replicas: 1
  selector:
    matchLabels:
      app: neo-node
  template:
    metadata:
      labels:
        app: neo-node
    spec:
      containers:
      - name: neo
        image: neo-cpp:latest
        ports:
        - containerPort: 10332
        - containerPort: 10333
        volumeMounts:
        - name: data
          mountPath: /var/lib/neo
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: neo-data-pvc
```

## Registry Operations

### Push to Registry

```bash
# Tag and push
make docker-tag
make docker-push
```

### Pull from Registry

```bash
make docker-pull
```

### Private Registry

Configure private registry:

```bash
cmake .. -DDOCKER_REGISTRY=myregistry.example.com
make docker
make docker-push
```

## Troubleshooting

### Common Issues

#### 1. Container Fails to Start

Check logs:
```bash
make docker-logs
```

Common causes:
- Invalid configuration file
- Port already in use
- Insufficient permissions

#### 2. Out of Disk Space

Clean up Docker resources:
```bash
docker system prune -a
make docker-clean-volumes
```

#### 3. Network Connectivity Issues

Test connectivity:
```bash
docker exec neo-cpp-node neo_cli_tool peers
```

#### 4. Permission Denied

Ensure proper file permissions:
```bash
chmod 755 docker-entrypoint.sh
chmod 644 config/*.json
```

### Debug Mode

Run with verbose logging:

```bash
docker run -e NEO_LOG_LEVEL=DEBUG \
           -e DEBUG=1 \
           --name neo-debug \
           neo-cpp:latest
```

### Performance Tuning

#### Container Optimization

```bash
# Use host networking for better performance
docker run --network=host neo-cpp:latest

# Increase shared memory
docker run --shm-size=2g neo-cpp:latest
```

#### Storage Optimization

Use volumes with specific drivers:

```bash
docker volume create --driver local \
    --opt type=none \
    --opt device=/fast/ssd/path \
    --opt o=bind \
    neo-fast-data
```

## Docker Image Details

### Image Layers

The Docker image uses a multi-stage build:

1. **Builder Stage**: Compiles Neo C++ from source
2. **Runtime Stage**: Minimal runtime with only necessary dependencies

### Installed Software

Runtime image includes:
- Ubuntu 22.04 base
- OpenSSL 3.x
- Boost libraries
- RocksDB
- Required compression libraries (lz4, zstd, snappy)

### File Structure

```
/opt/neo/           # Neo binaries
├── neo_node        # Main node executable
├── neo_cli_tool    # CLI tool
└── lib/            # Shared libraries

/etc/neo/           # Configuration
├── mainnet.json
├── testnet.json
└── private.json

/var/lib/neo/       # Blockchain data
/var/log/neo/       # Log files
```

## Advanced Configuration

### Custom Entrypoint

Override the entrypoint:

```bash
docker run --entrypoint /bin/bash neo-cpp:latest -c "neo_node --help"
```

### Environment File

Use an environment file:

```bash
# neo.env
NEO_NETWORK=testnet
NEO_LOG_LEVEL=DEBUG
NEO_DATA_PATH=/data

docker run --env-file neo.env neo-cpp:latest
```

### Docker Secrets

Use Docker secrets for sensitive data:

```bash
echo "mysecret" | docker secret create neo_wallet_password -
docker service create --secret neo_wallet_password neo-cpp:latest
```

## Monitoring and Metrics

### Prometheus Integration

The container exports metrics on port 9090:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'neo-node'
    static_configs:
      - targets: ['neo-node:9090']
```

### Grafana Dashboards

Import the provided Grafana dashboard:

1. Access Grafana at http://localhost:3000
2. Login with admin/admin
3. Import dashboard from `monitoring/grafana/dashboards/`

### Log Aggregation

Use log drivers for centralized logging:

```bash
docker run --log-driver=syslog \
           --log-opt syslog-address=tcp://logserver:514 \
           neo-cpp:latest
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Docker Build
on:
  push:
    branches: [main]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Build Docker image
      run: |
        mkdir build && cd build
        cmake ..
        make docker
    - name: Push to registry
      run: make docker-push
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                sh 'mkdir -p build && cd build && cmake ..'
                sh 'cd build && make docker'
            }
        }
        stage('Test') {
            steps {
                sh 'cd build && make docker-test'
            }
        }
        stage('Deploy') {
            steps {
                sh 'cd build && make docker-push'
            }
        }
    }
}
```

## Additional Resources

- [Docker Documentation](https://docs.docker.com)
- [Docker Compose Documentation](https://docs.docker.com/compose)
- [Neo Protocol Documentation](https://docs.neo.org)
- [CMake Documentation](https://cmake.org/documentation)

## Support

For issues related to Docker:
- Check the [Troubleshooting](#troubleshooting) section
- Review container logs with `make docker-logs`
- Open an issue on [GitHub](https://github.com/r3e-network/neo_cpp/issues)

---

*Last Updated: August 15, 2025*  
*Neo C++ Version: 1.2.0*