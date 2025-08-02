# Neo C++ Deployment Guide

This guide covers deployment options for the Neo C++ blockchain node in various environments.

## Table of Contents

- [System Requirements](#system-requirements)
- [Quick Start](#quick-start)
- [Installation Methods](#installation-methods)
- [Configuration](#configuration)
- [Running the Node](#running-the-node)
- [Monitoring](#monitoring)
- [Troubleshooting](#troubleshooting)
- [Security Considerations](#security-considerations)

## System Requirements

### Minimum Requirements

- **CPU**: 2 cores (x86_64 or ARM64)
- **RAM**: 4 GB
- **Storage**: 50 GB SSD
- **Network**: 100 Mbps
- **OS**: Ubuntu 20.04+, Debian 11+, RHEL 8+, macOS 11+, Windows Server 2019+

### Recommended Requirements

- **CPU**: 4+ cores
- **RAM**: 8+ GB
- **Storage**: 500 GB NVMe SSD
- **Network**: 1 Gbps
- **OS**: Ubuntu 22.04 LTS

### Network Ports

- **10332**: P2P communication (TCP)
- **10333**: WebSocket (TCP)
- **10334**: JSON-RPC (TCP)
- **10335**: HTTPS RPC (TCP, optional)
- **9090**: Prometheus metrics (TCP, optional)

## Quick Start

### Docker (Recommended)

```bash
# Run mainnet node
docker run -d \
  --name neo-node \
  -p 10332:10332 \
  -p 10334:10334 \
  -v neo-data:/var/lib/neo \
  neo-cpp:latest

# Check logs
docker logs -f neo-node

# Check sync status
curl http://localhost:10334 -d '{"jsonrpc":"2.0","method":"getblockcount","id":1}'
```

### Binary Installation

```bash
# Download latest release
wget https://github.com/neo-project/neo-cpp/releases/latest/download/neo-cpp-linux-amd64.tar.gz
tar xzf neo-cpp-linux-amd64.tar.gz

# Run node
./neo-node --config config.json
```

## Installation Methods

### 1. Docker Deployment

#### Using Docker Compose

```yaml
# docker-compose.yml
version: '3.8'

services:
  neo-node:
    image: neo-cpp:latest
    container_name: neo-mainnet
    restart: unless-stopped
    ports:
      - "10332:10332"
      - "10334:10334"
    volumes:
      - neo-data:/var/lib/neo
      - ./config:/etc/neo:ro
    environment:
      - NEO_NETWORK=mainnet
      - NEO_LOG_LEVEL=info
    healthcheck:
      test: ["CMD", "neo-cli", "rpc", "getblockcount"]
      interval: 30s
      timeout: 10s
      retries: 3

volumes:
  neo-data:
```

```bash
# Start the node
docker-compose up -d

# View logs
docker-compose logs -f

# Stop the node
docker-compose down
```

#### Building Docker Image

```bash
# Build from source
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp
docker build -t neo-cpp:latest .

# Multi-platform build
docker buildx build --platform linux/amd64,linux/arm64 -t neo-cpp:latest .
```

### 2. Kubernetes Deployment

#### Basic Deployment

```yaml
# neo-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: neo-node
  namespace: neo
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
          name: p2p
        - containerPort: 10334
          name: rpc
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
        volumeMounts:
        - name: neo-data
          mountPath: /var/lib/neo
        livenessProbe:
          httpGet:
            path: /health
            port: 10334
          initialDelaySeconds: 60
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /ready
            port: 10334
          initialDelaySeconds: 30
          periodSeconds: 10
      volumes:
      - name: neo-data
        persistentVolumeClaim:
          claimName: neo-data-pvc
```

```bash
# Deploy to Kubernetes
kubectl create namespace neo
kubectl apply -f neo-deployment.yaml

# Check status
kubectl get pods -n neo
kubectl logs -f deployment/neo-node -n neo
```

#### Helm Chart

```bash
# Add Neo Helm repository
helm repo add neo https://charts.neo.org
helm repo update

# Install Neo node
helm install neo-mainnet neo/neo-node \
  --namespace neo \
  --create-namespace \
  --set network=mainnet \
  --set persistence.size=500Gi \
  --set resources.requests.memory=4Gi
```

### 3. Systemd Service (Linux)

#### Installation Script

```bash
# Run deployment script
sudo ./scripts/deploy.sh --service --start

# Or manual installation
sudo useradd -r -s /bin/false neo
sudo mkdir -p /opt/neo-cpp /var/lib/neo /var/log/neo
sudo cp neo-node /opt/neo-cpp/
sudo chown -R neo:neo /opt/neo-cpp /var/lib/neo /var/log/neo
```

#### Service Configuration

```ini
# /etc/systemd/system/neo-node.service
[Unit]
Description=Neo C++ Blockchain Node
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=neo
Group=neo
WorkingDirectory=/opt/neo-cpp
ExecStart=/opt/neo-cpp/neo-node --config /etc/neo/config.json
Restart=always
RestartSec=10

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/neo /var/log/neo

# Resource limits
LimitNOFILE=65536
MemoryLimit=8G
CPUQuota=400%

[Install]
WantedBy=multi-user.target
```

```bash
# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable neo-node
sudo systemctl start neo-node

# Check status
sudo systemctl status neo-node
sudo journalctl -u neo-node -f
```

### 4. Building from Source

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  git \
  libboost-all-dev \
  libssl-dev \
  librocksdb-dev

# Clone and build
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## Configuration

### Basic Configuration

```json
{
  "ApplicationConfiguration": {
    "Network": "mainnet",
    "AddressVersion": 53,
    "Storage": {
      "Engine": "rocksdb",
      "Path": "/var/lib/neo/chain",
      "Options": {
        "WriteBufferSize": 67108864,
        "MaxOpenFiles": 1000,
        "BlockCacheSize": 134217728
      }
    },
    "P2P": {
      "Port": 10332,
      "MaxConnections": 100,
      "MinDesiredConnections": 10
    },
    "RPC": {
      "Enabled": true,
      "Port": 10334,
      "MaxConcurrentConnections": 40,
      "EnableCors": false
    },
    "Logging": {
      "Level": "info",
      "ConsoleEnabled": true,
      "FileEnabled": true,
      "Path": "/var/log/neo"
    }
  }
}
```

### Performance Tuning

```json
{
  "Performance": {
    "MaxTransactionsPerBlock": 512,
    "MemoryPoolSize": 50000,
    "ExecutionTimeout": 15,
    "MaxIteratorResultItems": 100,
    "UseMemoryPools": true,
    "ThreadPoolSize": 0,
    "EnableJIT": false
  }
}
```

### Network Selection

```bash
# Mainnet
neo-node --network mainnet

# Testnet
neo-node --network testnet

# Private network
neo-node --network private --config private-net.json
```

## Running the Node

### Command Line Options

```bash
neo-node [options]

Options:
  --config <file>       Configuration file path
  --network <name>      Network name (mainnet, testnet, private)
  --datadir <path>      Data directory path
  --rpc                 Enable RPC server
  --rpcport <port>      RPC server port
  --p2pport <port>      P2P port
  --nolog               Disable logging
  --daemon              Run as daemon
  --help                Show help message
```

### First Time Setup

```bash
# Initialize data directory
neo-node --datadir /var/lib/neo init

# Import chain data (optional, speeds up sync)
neo-node --datadir /var/lib/neo import chain.acc

# Start syncing
neo-node --config config.json
```

### Monitoring Sync Progress

```bash
# Check current height
curl -s http://localhost:10334 -d '{"jsonrpc":"2.0","method":"getblockcount","id":1}' | jq .result

# Check sync status
curl -s http://localhost:10334 -d '{"jsonrpc":"2.0","method":"getsyncstatus","id":1}' | jq .result

# Monitor in real-time
watch -n 1 'curl -s http://localhost:10334 -d "{\"jsonrpc\":\"2.0\",\"method\":\"getblockcount\",\"id\":1}" | jq .result'
```

## Monitoring

### Prometheus Metrics

```yaml
# prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'neo-node'
    static_configs:
    - targets: ['localhost:9090']
```

Available metrics:
- `neo_block_height`: Current blockchain height
- `neo_peer_count`: Number of connected peers
- `neo_mempool_size`: Memory pool transaction count
- `neo_rpc_requests_total`: Total RPC requests
- `neo_rpc_errors_total`: Total RPC errors

### Grafana Dashboard

Import the Neo monitoring dashboard:
1. Open Grafana
2. Go to Dashboards → Import
3. Use dashboard ID: `15991`

### Health Checks

```bash
# Basic health check
curl http://localhost:10334/health

# Detailed health check
curl http://localhost:10334/health/detailed
```

### Log Management

```bash
# View logs
tail -f /var/log/neo/node.log

# Log rotation configuration
cat > /etc/logrotate.d/neo <<EOF
/var/log/neo/*.log {
    daily
    missingok
    rotate 7
    compress
    delaycompress
    notifempty
    create 644 neo neo
    sharedscripts
    postrotate
        systemctl reload neo-node > /dev/null 2>&1 || true
    endscript
}
EOF
```

## Troubleshooting

### Common Issues

#### Node Won't Start

```bash
# Check logs
journalctl -u neo-node -n 100

# Check permissions
ls -la /var/lib/neo /var/log/neo

# Check ports
ss -tulpn | grep -E "10332|10334"

# Check disk space
df -h /var/lib/neo
```

#### Slow Synchronization

```bash
# Increase peer connections
neo-cli --config config.json set MaxConnections 200

# Check network connectivity
neo-cli peers

# Import bootstrap data
wget https://neo-bootstrap.com/chain.acc
neo-node import chain.acc
```

#### High Memory Usage

```bash
# Check memory usage
ps aux | grep neo-node

# Adjust memory limits
# In config.json:
{
  "Performance": {
    "MemoryPoolSize": 10000,
    "MaxCachedBlocks": 100
  }
}
```

### Debug Mode

```bash
# Run with debug logging
neo-node --loglevel debug

# Enable core dumps
ulimit -c unlimited
echo "/tmp/core.%e.%p" > /proc/sys/kernel/core_pattern

# Run with gdb
gdb neo-node
```

## Security Considerations

### Firewall Configuration

```bash
# UFW (Ubuntu)
sudo ufw allow 10332/tcp comment "Neo P2P"
sudo ufw allow 10334/tcp comment "Neo RPC" from 192.168.0.0/16

# iptables
sudo iptables -A INPUT -p tcp --dport 10332 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 10334 -s 192.168.0.0/16 -j ACCEPT
```

### RPC Security

```json
{
  "RPC": {
    "Enabled": true,
    "Port": 10334,
    "SslEnabled": true,
    "SslCertFile": "/etc/neo/cert.pem",
    "SslKeyFile": "/etc/neo/key.pem",
    "MaxGasInvoke": 100000000,
    "EnableAuth": true,
    "AuthToken": "your-secure-token",
    "AllowedOrigins": ["https://trusted-site.com"]
  }
}
```

### Best Practices

1. **Run as non-root user**
2. **Enable firewall rules**
3. **Use SSL/TLS for RPC**
4. **Regular security updates**
5. **Monitor logs for anomalies**
6. **Backup private keys securely**
7. **Use strong authentication**
8. **Limit RPC access**

### Security Checklist

- [ ] Firewall configured
- [ ] Non-root user created
- [ ] SSL/TLS enabled
- [ ] RPC authentication enabled
- [ ] Log monitoring setup
- [ ] Regular backups configured
- [ ] Security updates automated
- [ ] Resource limits set

## Backup and Recovery

### Backup Strategy

```bash
# Backup blockchain data
neo-node backup /backup/neo-backup-$(date +%Y%m%d).tar.gz

# Backup configuration
cp -r /etc/neo /backup/neo-config-$(date +%Y%m%d)

# Automated backup script
cat > /etc/cron.daily/neo-backup <<EOF
#!/bin/bash
neo-node backup /backup/neo-backup-\$(date +%Y%m%d).tar.gz
find /backup -name "neo-backup-*.tar.gz" -mtime +7 -delete
EOF
chmod +x /etc/cron.daily/neo-backup
```

### Recovery Process

```bash
# Stop node
systemctl stop neo-node

# Restore from backup
tar xzf /backup/neo-backup-20240115.tar.gz -C /var/lib/neo

# Start node
systemctl start neo-node
```

## Performance Optimization

### System Tuning

```bash
# Increase file descriptors
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# Network tuning
cat >> /etc/sysctl.conf <<EOF
net.core.somaxconn = 1024
net.ipv4.tcp_max_syn_backlog = 2048
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 1200
EOF
sysctl -p
```

### Storage Optimization

```bash
# Use fast NVMe storage
lsblk -d -o name,rota | grep " 0$"

# Enable TRIM for SSDs
systemctl enable fstrim.timer

# Mount options
echo "/dev/nvme0n1p1 /var/lib/neo ext4 defaults,noatime,nodiratime 0 2" >> /etc/fstab
```

## Support

For additional help:

- **Documentation**: https://docs.neo.org
- **GitHub Issues**: https://github.com/neo-project/neo-cpp/issues
- **Discord**: https://discord.gg/neo
- **Forum**: https://github.com/neo-project/neo/discussions