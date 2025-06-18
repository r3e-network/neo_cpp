# Neo C++ Deployment Guide

## Overview

This guide covers the deployment of the Neo C++ blockchain node in various environments, from development setups to production deployments.

## System Requirements

### Minimum Requirements
- **OS**: Ubuntu 20.04 LTS, CentOS 8, or Windows 10/11
- **CPU**: 4 cores, 2.5 GHz
- **RAM**: 8 GB
- **Storage**: 100 GB SSD
- **Network**: 10 Mbps broadband connection

### Recommended Requirements
- **OS**: Ubuntu 22.04 LTS or CentOS Stream 9
- **CPU**: 8 cores, 3.0 GHz
- **RAM**: 16 GB
- **Storage**: 500 GB NVMe SSD
- **Network**: 100 Mbps fiber connection

### Production Requirements
- **OS**: Ubuntu 22.04 LTS (Server)
- **CPU**: 16 cores, 3.5 GHz
- **RAM**: 32 GB
- **Storage**: 1 TB NVMe SSD (RAID 1)
- **Network**: 1 Gbps dedicated connection
- **Backup**: Additional 2 TB for backups

## Dependencies

### Required Dependencies
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libjson-c-dev \
    libleveldb-dev \
    libboost-all-dev \
    libgmp-dev \
    libsecp256k1-dev

# CentOS/RHEL
sudo dnf groupinstall "Development Tools"
sudo dnf install -y \
    cmake \
    git \
    pkgconfig \
    openssl-devel \
    json-c-devel \
    leveldb-devel \
    boost-devel \
    gmp-devel \
    libsecp256k1-devel
```

### vcpkg Dependencies
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Install required packages
./vcpkg install \
    openssl \
    boost-system \
    boost-filesystem \
    boost-program-options \
    boost-thread \
    leveldb \
    nlohmann-json \
    secp256k1 \
    gmp \
    catch2
```

## Build Instructions

### Development Build
```bash
# Clone repository
git clone https://github.com/r3e-network/neo_cpp.git
cd neo_cpp

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### Release Build
```bash
# Configure for release
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DBUILD_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_INSTALL_PREFIX=/opt/neo

# Build and install
make -j$(nproc)
sudo make install
```

### Production Build with Optimizations
```bash
# Configure for production
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG" \
    -DBUILD_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DENABLE_LTO=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/neo

# Build with maximum optimization
make -j$(nproc)
sudo make install
```

## Configuration

### Basic Configuration
Create `/opt/neo/config/protocol.json`:
```json
{
  "Network": 860833102,
  "AddressVersion": 53,
  "MillisecondsPerBlock": 15000,
  "MaxTransactionsPerBlock": 512,
  "MemoryPoolMaxTransactions": 50000,
  "MaxTraceableBlocks": 2102400,
  "ValidatorsCount": 7,
  "CommitteeMembersCount": 21,
  "SeedList": [
    "seed1.neo.org:10333",
    "seed2.neo.org:10333",
    "seed3.neo.org:10333",
    "seed4.neo.org:10333",
    "seed5.neo.org:10333"
  ],
  "StandbyCommittee": [
    "0c40c10f101415d0e4b2b50b4af7c8b5e6dbcd1e9a9f0e1f1a1c1e1f1a1c1e1f",
    "...additional committee members..."
  ]
}
```

### Network Configuration
Create `/opt/neo/config/network.json`:
```json
{
  "P2P": {
    "Port": 10333,
    "WsPort": 10334,
    "MaxConnections": 40,
    "MaxConnectionsPerAddress": 3
  },
  "RPC": {
    "Port": 10332,
    "BindAddress": "127.0.0.1",
    "MaxConnections": 40,
    "TrustProxy": false,
    "DisabledMethods": []
  },
  "UnlockWallet": {
    "Path": "",
    "Password": "",
    "IsActive": false
  }
}
```

### Storage Configuration
Create `/opt/neo/config/storage.json`:
```json
{
  "Engine": "LevelDB",
  "Path": "/opt/neo/data/chain",
  "Options": {
    "CreateIfMissing": true,
    "ErrorIfExists": false,
    "WriteBufferSize": 4194304,
    "MaxOpenFiles": 1000,
    "BlockSize": 4096,
    "BlockRestartInterval": 16,
    "Compression": "Snappy"
  }
}
```

## Deployment Scenarios

### Single Node Deployment

#### Development Environment
```bash
# Start development node
/opt/neo/bin/neo-cli \
    --config /opt/neo/config/protocol.json \
    --network testnet \
    --rpc-port 20332 \
    --p2p-port 20333 \
    --data-dir ./data
```

#### Testnet Node
```bash
# Create systemd service
sudo tee /etc/systemd/system/neo-testnet.service << EOF
[Unit]
Description=Neo C++ Testnet Node
After=network.target

[Service]
Type=simple
User=neo
Group=neo
WorkingDirectory=/opt/neo
ExecStart=/opt/neo/bin/neo-cli --config /opt/neo/config/testnet.json
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable neo-testnet
sudo systemctl start neo-testnet
```

#### Mainnet Node
```bash
# Create production systemd service
sudo tee /etc/systemd/system/neo-mainnet.service << EOF
[Unit]
Description=Neo C++ Mainnet Node
After=network.target

[Service]
Type=simple
User=neo
Group=neo
WorkingDirectory=/opt/neo
ExecStart=/opt/neo/bin/neo-cli --config /opt/neo/config/mainnet.json
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
LimitNOFILE=65536
LimitCORE=infinity

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable neo-mainnet
sudo systemctl start neo-mainnet
```

### High Availability Deployment

#### Load Balanced RPC Nodes
```bash
# Node 1
/opt/neo/bin/neo-cli \
    --config /opt/neo/config/mainnet.json \
    --rpc-bind 0.0.0.0 \
    --rpc-port 10332 \
    --instance-id node1

# Node 2
/opt/neo/bin/neo-cli \
    --config /opt/neo/config/mainnet.json \
    --rpc-bind 0.0.0.0 \
    --rpc-port 10332 \
    --instance-id node2

# Node 3
/opt/neo/bin/neo-cli \
    --config /opt/neo/config/mainnet.json \
    --rpc-bind 0.0.0.0 \
    --rpc-port 10332 \
    --instance-id node3
```

#### Nginx Load Balancer Configuration
```nginx
upstream neo_rpc_backend {
    server 10.0.1.10:10332 weight=1;
    server 10.0.1.11:10332 weight=1;
    server 10.0.1.12:10332 weight=1;
}

server {
    listen 80;
    server_name neo-rpc.example.com;
    
    location / {
        proxy_pass http://neo_rpc_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
}
```

### Consensus Node Deployment

#### Validator Node Setup
```bash
# Create validator configuration
cat > /opt/neo/config/validator.json << EOF
{
  "Network": 860833102,
  "ConsensusEnabled": true,
  "ValidatorKey": "your_validator_private_key_here",
  "UnlockWallet": {
    "Path": "/opt/neo/wallets/validator.json",
    "Password": "your_secure_password"
  }
}
EOF

# Start validator node
/opt/neo/bin/neo-cli \
    --config /opt/neo/config/validator.json \
    --consensus \
    --unlock-wallet
```

#### Committee Member Setup
```bash
# Configure committee member
cat > /opt/neo/config/committee.json << EOF
{
  "Network": 860833102,
  "CommitteeEnabled": true,
  "CommitteeKey": "your_committee_private_key_here",
  "OracleEnabled": true
}
EOF
```

## Container Deployment

### Docker Setup
```dockerfile
# Dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libleveldb-dev \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy application
COPY --from=builder /opt/neo /opt/neo

# Create user
RUN useradd -r -s /bin/false neo
RUN chown -R neo:neo /opt/neo

# Expose ports
EXPOSE 10332 10333

# Start node
USER neo
WORKDIR /opt/neo
CMD ["/opt/neo/bin/neo-cli", "--config", "/opt/neo/config/mainnet.json"]
```

### Docker Compose
```yaml
version: '3.8'

services:
  neo-node:
    build: .
    ports:
      - "10332:10332"
      - "10333:10333"
    volumes:
      - neo_data:/opt/neo/data
      - ./config:/opt/neo/config:ro
    restart: unless-stopped
    environment:
      - NEO_NETWORK=mainnet
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:10332"]
      interval: 30s
      timeout: 10s
      retries: 3

  neo-monitor:
    image: prom/prometheus
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
    restart: unless-stopped

volumes:
  neo_data:
```

### Kubernetes Deployment
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: neo-node
spec:
  replicas: 3
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
        - containerPort: 10332
        - containerPort: 10333
        volumeMounts:
        - name: config
          mountPath: /opt/neo/config
        - name: data
          mountPath: /opt/neo/data
        resources:
          requests:
            memory: "2Gi"
            cpu: "1000m"
          limits:
            memory: "4Gi"
            cpu: "2000m"
      volumes:
      - name: config
        configMap:
          name: neo-config
      - name: data
        persistentVolumeClaim:
          claimName: neo-data
---
apiVersion: v1
kind: Service
metadata:
  name: neo-rpc-service
spec:
  selector:
    app: neo-node
  ports:
  - port: 10332
    targetPort: 10332
  type: LoadBalancer
```

## Security Considerations

### Network Security
```bash
# Firewall configuration
sudo ufw enable
sudo ufw default deny incoming
sudo ufw default allow outgoing

# Allow Neo ports
sudo ufw allow 10332/tcp comment 'Neo RPC'
sudo ufw allow 10333/tcp comment 'Neo P2P'

# Allow SSH (change default port)
sudo ufw allow 2222/tcp comment 'SSH'
```

### File System Security
```bash
# Set proper permissions
sudo chown -R neo:neo /opt/neo
sudo chmod 750 /opt/neo
sudo chmod 600 /opt/neo/config/*.json
sudo chmod 700 /opt/neo/wallets/
```

### SSL/TLS Configuration
```nginx
server {
    listen 443 ssl http2;
    server_name neo-rpc.example.com;
    
    ssl_certificate /path/to/certificate.pem;
    ssl_certificate_key /path/to/private.key;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM:DHE+CHACHA20:!aNULL:!MD5:!DSS;
    
    location / {
        proxy_pass http://127.0.0.1:10332;
        proxy_ssl_verify off;
    }
}
```

## Monitoring and Logging

### Log Configuration
```bash
# Configure log rotation
sudo tee /etc/logrotate.d/neo << EOF
/opt/neo/logs/*.log {
    daily
    missingok
    rotate 30
    compress
    delaycompress
    notifempty
    copytruncate
    postrotate
        systemctl reload neo-mainnet
    endscript
}
EOF
```

### Prometheus Monitoring
```yaml
# prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'neo-node'
    static_configs:
      - targets: ['localhost:10332']
    metrics_path: /metrics
    scrape_interval: 30s
```

### Health Check Script
```bash
#!/bin/bash
# health_check.sh

NEO_RPC_URL="http://localhost:10332"
EXPECTED_NETWORK=860833102

# Check if RPC is responding
response=$(curl -s -X POST "$NEO_RPC_URL" \
    -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}')

if [ $? -ne 0 ]; then
    echo "ERROR: RPC not responding"
    exit 1
fi

# Check network
network=$(echo "$response" | jq -r '.result.network')
if [ "$network" != "$EXPECTED_NETWORK" ]; then
    echo "ERROR: Wrong network: $network"
    exit 1
fi

# Check block height (should be increasing)
height=$(curl -s -X POST "$NEO_RPC_URL" \
    -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' | \
    jq -r '.result')

echo "OK: Network $network, Height $height"
exit 0
```

## Backup and Recovery

### Automated Backup Script
```bash
#!/bin/bash
# backup.sh

BACKUP_DIR="/backup/neo"
DATA_DIR="/opt/neo/data"
RETENTION_DAYS=30

# Create backup directory
mkdir -p "$BACKUP_DIR"

# Stop node
systemctl stop neo-mainnet

# Create backup
tar -czf "$BACKUP_DIR/neo-backup-$(date +%Y%m%d_%H%M%S).tar.gz" \
    -C "$(dirname $DATA_DIR)" "$(basename $DATA_DIR)"

# Start node
systemctl start neo-mainnet

# Cleanup old backups
find "$BACKUP_DIR" -name "neo-backup-*.tar.gz" -mtime +$RETENTION_DAYS -delete

echo "Backup completed successfully"
```

### Recovery Procedure
```bash
#!/bin/bash
# restore.sh

BACKUP_FILE="$1"
DATA_DIR="/opt/neo/data"

if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup_file>"
    exit 1
fi

# Stop node
systemctl stop neo-mainnet

# Backup current data
mv "$DATA_DIR" "${DATA_DIR}.bak"

# Restore from backup
tar -xzf "$BACKUP_FILE" -C "$(dirname $DATA_DIR)"

# Start node
systemctl start neo-mainnet

echo "Recovery completed successfully"
```

## Performance Tuning

### System Optimizations
```bash
# Increase file descriptor limits
echo "neo soft nofile 65536" >> /etc/security/limits.conf
echo "neo hard nofile 65536" >> /etc/security/limits.conf

# Optimize TCP settings
echo "net.core.somaxconn = 65536" >> /etc/sysctl.conf
echo "net.core.netdev_max_backlog = 5000" >> /etc/sysctl.conf
echo "net.ipv4.tcp_max_syn_backlog = 65536" >> /etc/sysctl.conf

# Apply settings
sysctl -p
```

### Database Tuning
```json
{
  "Storage": {
    "Engine": "LevelDB",
    "Options": {
      "WriteBufferSize": 67108864,
      "MaxOpenFiles": 2000,
      "BlockSize": 65536,
      "MaxFileSize": 2097152,
      "Compression": "Snappy",
      "FilterPolicy": "bloom:10"
    }
  }
}
```

## Troubleshooting

### Common Issues

#### Port Already in Use
```bash
# Find process using port
sudo netstat -tulpn | grep :10332
sudo lsof -i :10332

# Kill process if necessary
sudo kill -9 <pid>
```

#### Blockchain Sync Issues
```bash
# Check sync status
curl -X POST http://localhost:10332 \
    -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}'

# Reset blockchain data
systemctl stop neo-mainnet
rm -rf /opt/neo/data/chain
systemctl start neo-mainnet
```

#### Memory Issues
```bash
# Monitor memory usage
htop
free -h

# Check for memory leaks
valgrind --tool=memcheck --leak-check=full /opt/neo/bin/neo-cli
```

This deployment guide provides comprehensive instructions for deploying the Neo C++ node in various environments with proper security, monitoring, and maintenance procedures.