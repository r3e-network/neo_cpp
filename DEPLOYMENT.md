# Neo C++ Deployment Guide

This guide covers deployment options, configuration, and best practices for running Neo C++ nodes in production.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Installation](#installation)
3. [Node Types](#node-types)
4. [Configuration](#configuration)
5. [Running in Production](#running-in-production)
6. [Monitoring](#monitoring)
7. [Security](#security)
8. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

- **CPU**: 2 cores (x86_64 or ARM64)
- **RAM**: 4 GB
- **Storage**: 50 GB SSD
- **Network**: 100 Mbps
- **OS**: Linux (Ubuntu 20.04+), Windows Server 2019+, macOS 11+

### Recommended Production Requirements

- **CPU**: 8+ cores
- **RAM**: 16 GB
- **Storage**: 500 GB NVMe SSD
- **Network**: 1 Gbps
- **OS**: Ubuntu 22.04 LTS or RHEL 8+

## Installation

### From Source

```bash
# Clone repository
git clone https://github.com/r3e-network/neo_cpp.git
cd neo_cpp

# Build with optimizations
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEO_BUILD_TESTS=OFF
make -j$(nproc)
sudo make install
```

### Docker

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libssl-dev libboost-all-dev

COPY . /neo-cpp
WORKDIR /neo-cpp/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

EXPOSE 10333 10332
CMD ["neo_node_app", "/config/config.json"]
```

### Package Managers

```bash
# Ubuntu/Debian (coming soon)
sudo apt install neo-cpp

# RHEL/CentOS (coming soon)
sudo yum install neo-cpp
```

## Node Types

### 1. Simple Neo Node

Lightweight implementation for development and testing:

```bash
./simple_neo_node

# Features:
# - In-memory storage
# - Basic wallet functionality
# - Interactive CLI
# - Minimal resource usage
```

### 2. Working Neo Node

Full-featured implementation for testing environments:

```bash
./working_neo_node

# Features:
# - Persistent storage
# - Complete transaction processing
# - Smart contract execution
# - Basic networking
```

### 3. Production Neo Node (neo_node_app)

Enterprise-grade implementation:

```bash
./neo_node_app config.json

# Features:
# - Full consensus participation
# - RPC/REST API server
# - Plugin system
# - High availability
# - Monitoring integration
```

## Configuration

### Basic Configuration (config.json)

```json
{
  "ApplicationConfiguration": {
    "DataPath": "./data",
    "Network": "mainnet",
    "Storage": {
      "Engine": "RocksDB",
      "Path": "./data/rocksdb"
    },
    "P2P": {
      "Port": 10333,
      "MaxConnections": 100,
      "MinDesiredConnections": 10
    },
    "RPC": {
      "Enabled": true,
      "Port": 10332,
      "MaxConcurrentConnections": 40,
      "MaxGasInvoke": 100000000
    }
  },
  "ProtocolConfiguration": {
    "Network": 860833102,
    "AddressVersion": 53,
    "StandbyCommittee": [
      "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
      "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a"
    ],
    "SeedList": [
      "seed1.neo.org:10333",
      "seed2.neo.org:10333",
      "seed3.neo.org:10333"
    ]
  }
}
```

### Advanced Configuration

```json
{
  "ApplicationConfiguration": {
    "Logger": {
      "Path": "./logs",
      "ConsoleOutput": true,
      "Active": true,
      "Level": "info"
    },
    "Storage": {
      "Engine": "RocksDB",
      "Options": {
        "CacheSize": 536870912,
        "WriteBufferSize": 67108864,
        "MaxOpenFiles": 1000,
        "CompressionType": "lz4"
      }
    },
    "UnlockWallet": {
      "Path": "./wallet.json",
      "Password": "${WALLET_PASSWORD}",
      "IsActive": true
    },
    "Consensus": {
      "Enabled": true,
      "MaxTransactionsPerBlock": 500,
      "MillisecondsPerBlock": 15000
    },
    "Plugins": {
      "RpcServer": {
        "Enabled": true,
        "Network": "0.0.0.0",
        "Port": 10332,
        "SslEnabled": true,
        "SslCertFile": "./certs/server.crt",
        "SslKeyFile": "./certs/server.key"
      },
      "ApplicationLogs": {
        "Enabled": true,
        "Path": "./applogs"
      },
      "StatesDumper": {
        "Enabled": false,
        "HeightToBegin": 0,
        "HeightToEnd": 0
      }
    }
  }
}
```

## Running in Production

### Systemd Service

Create `/etc/systemd/system/neo-node.service`:

```ini
[Unit]
Description=Neo C++ Node
After=network.target

[Service]
Type=simple
User=neo
Group=neo
WorkingDirectory=/opt/neo
ExecStart=/opt/neo/neo_node_app /opt/neo/config.json
Restart=always
RestartSec=30
StandardOutput=append:/var/log/neo/node.log
StandardError=append:/var/log/neo/error.log

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/neo/data /var/log/neo

# Resource limits
LimitNOFILE=65536
MemoryLimit=8G
CPUQuota=400%

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable neo-node
sudo systemctl start neo-node
```

### Docker Compose

```yaml
version: '3.8'

services:
  neo-node:
    image: neo-cpp:latest
    container_name: neo-mainnet
    restart: unless-stopped
    ports:
      - "10333:10333"  # P2P
      - "10332:10332"  # RPC
    volumes:
      - ./data:/data
      - ./config:/config
      - ./logs:/logs
    environment:
      - NEO_NETWORK=mainnet
      - WALLET_PASSWORD=${WALLET_PASSWORD}
    ulimits:
      nofile:
        soft: 65536
        hard: 65536
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:10332/health"]
      interval: 30s
      timeout: 10s
      retries: 3

  prometheus:
    image: prom/prometheus
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9090:9090"

  grafana:
    image: grafana/grafana
    ports:
      - "3000:3000"
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
```

### Kubernetes Deployment

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: neo-node
spec:
  serviceName: neo-node
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
      - name: neo-node
        image: neo-cpp:latest
        ports:
        - containerPort: 10333
          name: p2p
        - containerPort: 10332
          name: rpc
        volumeMounts:
        - name: data
          mountPath: /data
        - name: config
          mountPath: /config
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
        livenessProbe:
          httpGet:
            path: /health
            port: 10332
          initialDelaySeconds: 60
          periodSeconds: 30
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: "fast-ssd"
      resources:
        requests:
          storage: 500Gi
```

## Monitoring

### Prometheus Metrics

Configure Prometheus to scrape metrics:

```yaml
scrape_configs:
  - job_name: 'neo-node'
    static_configs:
      - targets: ['localhost:9103']
    metrics_path: '/metrics'
```

### Key Metrics to Monitor

- **Blockchain**
  - `neo_block_height` - Current block height
  - `neo_block_time` - Block processing time
  - `neo_tx_pool_size` - Transaction pool size

- **Network**
  - `neo_peers_connected` - Number of connected peers
  - `neo_bandwidth_bytes` - Network bandwidth usage
  - `neo_message_received_total` - Messages received

- **Performance**
  - `neo_vm_execution_time` - VM execution time
  - `neo_storage_read_latency` - Storage read latency
  - `neo_memory_usage_bytes` - Memory usage

### Grafana Dashboard

Import the Neo C++ dashboard (ID: 12345) or create custom dashboards.

## Security

### Network Security

1. **Firewall Rules**
   ```bash
   # Allow P2P
   sudo ufw allow 10333/tcp
   
   # Allow RPC (restrict source IPs)
   sudo ufw allow from 192.168.1.0/24 to any port 10332
   ```

2. **SSL/TLS for RPC**
   ```json
   {
     "RPC": {
       "SslEnabled": true,
       "SslCertFile": "./certs/server.crt",
       "SslKeyFile": "./certs/server.key"
     }
   }
   ```

3. **API Authentication**
   ```json
   {
     "RPC": {
       "RequireAuthentication": true,
       "AuthTokens": ["${API_TOKEN}"]
     }
   }
   ```

### Wallet Security

1. **Encrypted Storage**
   - Always use strong passwords
   - Store passwords in secure vault
   - Enable hardware wallet support

2. **Key Management**
   ```bash
   # Generate secure wallet
   ./neo-cli wallet create --encrypted
   
   # Use hardware wallet
   ./neo-cli wallet import --ledger
   ```

### System Security

1. **Run as Non-Root User**
   ```bash
   sudo useradd -m -s /bin/bash neo
   sudo chown -R neo:neo /opt/neo
   ```

2. **File Permissions**
   ```bash
   chmod 600 /opt/neo/config.json
   chmod 700 /opt/neo/data
   ```

3. **Regular Updates**
   ```bash
   # Check for updates
   ./neo-cli version --check-update
   
   # Update
   sudo systemctl stop neo-node
   # ... update binaries ...
   sudo systemctl start neo-node
   ```

## Troubleshooting

### Common Issues

1. **Node Won't Sync**
   - Check network connectivity
   - Verify seed nodes are reachable
   - Check disk space
   - Review logs for errors

2. **High Memory Usage**
   - Adjust cache sizes in configuration
   - Enable memory limits in systemd
   - Check for memory leaks

3. **RPC Not Responding**
   - Verify RPC is enabled
   - Check firewall rules
   - Review RPC logs
   - Test with curl: `curl http://localhost:10332/health`

### Debug Mode

Enable debug logging:

```json
{
  "Logger": {
    "Level": "debug",
    "ConsoleOutput": true
  }
}
```

### Performance Tuning

1. **Storage Optimization**
   ```json
   {
     "Storage": {
       "Options": {
         "CacheSize": 1073741824,
         "WriteBufferSize": 134217728,
         "MaxOpenFiles": 2000,
         "BlockCache": true
       }
     }
   }
   ```

2. **Network Optimization**
   ```bash
   # Increase file descriptors
   echo "neo soft nofile 65536" >> /etc/security/limits.conf
   echo "neo hard nofile 65536" >> /etc/security/limits.conf
   
   # TCP tuning
   echo "net.core.somaxconn = 1024" >> /etc/sysctl.conf
   echo "net.ipv4.tcp_tw_reuse = 1" >> /etc/sysctl.conf
   ```

### Backup and Recovery

1. **Automated Backups**
   ```bash
   #!/bin/bash
   # backup.sh
   BACKUP_DIR="/backup/neo-$(date +%Y%m%d)"
   mkdir -p $BACKUP_DIR
   
   # Stop node
   systemctl stop neo-node
   
   # Backup data
   cp -r /opt/neo/data $BACKUP_DIR/
   cp /opt/neo/config.json $BACKUP_DIR/
   
   # Start node
   systemctl start neo-node
   
   # Compress and encrypt
   tar -czf - $BACKUP_DIR | openssl enc -aes-256-cbc -out $BACKUP_DIR.tar.gz.enc
   ```

2. **Recovery**
   ```bash
   # Decrypt and extract
   openssl enc -d -aes-256-cbc -in backup.tar.gz.enc | tar -xzf -
   
   # Restore
   systemctl stop neo-node
   cp -r backup/data/* /opt/neo/data/
   systemctl start neo-node
   ```

## Support

- **Documentation**: https://github.com/r3e-network/neo_cpp/wiki
- **Issues**: https://github.com/r3e-network/neo_cpp/issues
- **Community**: Discord #neo-cpp channel
- **Commercial Support**: support@r3e.network