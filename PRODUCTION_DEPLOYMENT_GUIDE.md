# Neo C++ Production Deployment Guide

## Table of Contents
1. [System Requirements](#system-requirements)
2. [Installation](#installation)
3. [Configuration](#configuration)
4. [Deployment](#deployment)
5. [Operations](#operations)
6. [Monitoring](#monitoring)
7. [Security](#security)
8. [Troubleshooting](#troubleshooting)

## System Requirements

### Hardware Requirements

**Minimum (Testnet)**
- CPU: 4 cores @ 2.4GHz
- RAM: 16GB
- Storage: 500GB SSD
- Network: 100Mbps

**Recommended (Mainnet)**
- CPU: 8+ cores @ 3.0GHz
- RAM: 32GB
- Storage: 1TB NVMe SSD
- Network: 1Gbps

**Production (Consensus Node)**
- CPU: 16+ cores @ 3.5GHz
- RAM: 64GB
- Storage: 2TB NVMe SSD RAID
- Network: 10Gbps

### Software Requirements
- OS: Ubuntu 20.04 LTS or later
- Compiler: GCC 10+ or Clang 12+
- CMake: 3.20+
- Dependencies: See `setup-vcpkg.sh`

## Installation

### 1. Clone Repository
```bash
git clone https://github.com/your-org/neo-cpp.git
cd neo-cpp
```

### 2. Install Dependencies
```bash
# Install system packages
sudo apt update
sudo apt install -y build-essential cmake git curl zip unzip

# Setup vcpkg dependencies
./setup-vcpkg.sh
```

### 3. Build from Source
```bash
# Create build directory
mkdir build && cd build

# Configure with vcpkg
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DNEO_PRODUCTION_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake

# Build (use all cores)
make -j$(nproc)

# Run tests
make test

# Install
sudo make install
```

### 4. Docker Deployment
```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y \
    build-essential cmake git curl zip unzip
COPY . /neo-cpp
WORKDIR /neo-cpp
RUN ./setup-vcpkg.sh && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install
EXPOSE 10332 10333 8080 9090
CMD ["/usr/local/bin/neo-node"]
```

## Configuration

### 1. Basic Configuration
```bash
# Copy default configuration
cp config/production_config.yaml /etc/neo/config.yaml

# Edit configuration
nano /etc/neo/config.yaml
```

### 2. Environment Variables
```bash
# Network selection
export NEO_NETWORK=mainnet

# Database path
export NEO_DB_PATH=/var/lib/neo/chain

# Log level
export NEO_LOG_LEVEL=info

# RPC settings
export NEO_RPC_ENABLED=true
export NEO_RPC_PORT=10332
```

### 3. Systemd Service
```ini
[Unit]
Description=Neo C++ Node
After=network.target

[Service]
Type=simple
User=neo
Group=neo
WorkingDirectory=/var/lib/neo
ExecStart=/usr/local/bin/neo-node --config /etc/neo/config.yaml
Restart=always
RestartSec=10
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

### 4. Security Hardening
```bash
# Create dedicated user
sudo useradd -r -s /bin/false neo

# Set permissions
sudo chown -R neo:neo /var/lib/neo
sudo chmod 700 /var/lib/neo

# Configure firewall
sudo ufw allow 10333/tcp  # P2P
sudo ufw allow 10332/tcp  # RPC (if public)
```

## Deployment

### 1. Initial Sync
```bash
# Start node in sync mode
neo-node --config /etc/neo/config.yaml --sync-only

# Monitor sync progress
neo-cli getblockcount
```

### 2. Production Checklist
- [ ] Hardware meets requirements
- [ ] OS and dependencies updated
- [ ] Configuration reviewed
- [ ] Firewall rules configured
- [ ] Monitoring setup
- [ ] Backup strategy defined
- [ ] Security audit completed
- [ ] Load testing performed

### 3. High Availability Setup
```yaml
# HAProxy configuration for RPC load balancing
global
    maxconn 4096
    
defaults
    mode http
    timeout connect 5000ms
    timeout client 50000ms
    timeout server 50000ms
    
frontend neo_rpc
    bind *:10332
    default_backend neo_nodes
    
backend neo_nodes
    balance roundrobin
    server node1 10.0.0.1:10332 check
    server node2 10.0.0.2:10332 check
    server node3 10.0.0.3:10332 check
```

## Operations

### 1. Daily Tasks
- Check node sync status
- Monitor disk space
- Review error logs
- Verify peer connections

### 2. Backup Procedures
```bash
# Online backup using checkpoint
neo-cli checkpoint /backup/neo-checkpoint-$(date +%Y%m%d)

# Offline backup
systemctl stop neo-node
rsync -av /var/lib/neo/chain /backup/
systemctl start neo-node
```

### 3. Upgrade Procedures
```bash
# 1. Backup current state
./backup.sh

# 2. Download new version
wget https://github.com/your-org/neo-cpp/releases/neo-cpp-v2.0.0.tar.gz

# 3. Stop node
systemctl stop neo-node

# 4. Install new version
tar xzf neo-cpp-v2.0.0.tar.gz
cd neo-cpp-v2.0.0
./install.sh

# 5. Start node
systemctl start neo-node

# 6. Verify operation
neo-cli getversion
```

## Monitoring

### 1. Prometheus Metrics
```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'neo-node'
    static_configs:
      - targets: ['localhost:9090']
```

### 2. Key Metrics to Monitor
- `neo_block_height` - Current blockchain height
- `neo_peer_count` - Connected peer count
- `neo_transaction_pool_size` - Pending transactions
- `neo_rpc_requests_total` - RPC request count
- `neo_rpc_errors_total` - RPC error count
- `neo_db_read_latency` - Database read performance
- `neo_consensus_view` - Consensus view number

### 3. Alerting Rules
```yaml
groups:
  - name: neo_alerts
    rules:
      - alert: NodeNotSyncing
        expr: increase(neo_block_height[5m]) == 0
        for: 10m
        
      - alert: LowPeerCount
        expr: neo_peer_count < 3
        for: 5m
        
      - alert: HighRPCErrors
        expr: rate(neo_rpc_errors_total[5m]) > 0.1
        for: 5m
        
      - alert: DiskSpaceLow
        expr: node_filesystem_avail_bytes{mountpoint="/var/lib/neo"} < 50e9
        for: 10m
```

### 4. Health Check Endpoints
- `GET /health` - Overall health status
- `GET /health/liveness` - Is node alive
- `GET /health/readiness` - Is node ready for traffic
- `GET /metrics` - Prometheus metrics

## Security

### 1. Network Security
```bash
# IPTables rules
iptables -A INPUT -p tcp --dport 10333 -m connlimit --connlimit-above 50 -j REJECT
iptables -A INPUT -p tcp --dport 10332 -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 10332 -j DROP
```

### 2. TLS Configuration
```yaml
security:
  enable_tls: true
  tls_cert_file: /etc/neo/certs/server.crt
  tls_key_file: /etc/neo/certs/server.key
  tls_ca_file: /etc/neo/certs/ca.crt
```

### 3. Access Control
```nginx
# Nginx reverse proxy for RPC
location /rpc {
    auth_basic "Neo RPC";
    auth_basic_user_file /etc/nginx/.htpasswd;
    
    proxy_pass http://localhost:10332;
    proxy_set_header X-Real-IP $remote_addr;
    
    # Rate limiting
    limit_req zone=rpc burst=10 nodelay;
}
```

## Troubleshooting

### Common Issues

**1. Node Not Syncing**
```bash
# Check peer count
neo-cli getpeers

# Check network connectivity
nc -zv seed1.neo.org 10333

# Review logs
tail -f /var/log/neo/neo.log | grep -i error
```

**2. High Memory Usage**
```bash
# Check memory stats
neo-cli getmemorypool

# Clear memory pool if needed
neo-cli clearmemorypool

# Adjust cache sizes in config
```

**3. Database Corruption**
```bash
# Verify database integrity
neo-cli verifychain

# Rebuild from checkpoint
systemctl stop neo-node
rm -rf /var/lib/neo/chain
neo-cli restore /backup/checkpoint
systemctl start neo-node
```

### Performance Tuning

**1. Database Optimization**
```yaml
database:
  cache_size: 2048  # Increase for better performance
  write_buffer_size: 256
  max_open_files: 10000
  use_bloom_filter: true
```

**2. Network Optimization**
```bash
# Increase file descriptors
echo "neo soft nofile 65536" >> /etc/security/limits.conf
echo "neo hard nofile 65536" >> /etc/security/limits.conf

# TCP tuning
sysctl -w net.core.somaxconn=1024
sysctl -w net.ipv4.tcp_max_syn_backlog=2048
```

**3. CPU Affinity**
```bash
# Bind to specific CPU cores
taskset -c 0-7 neo-node
```

## Support

- Documentation: https://docs.neo.org
- GitHub Issues: https://github.com/your-org/neo-cpp/issues
- Community: https://discord.gg/neo

---

**Important**: Always test configuration changes in a non-production environment first.