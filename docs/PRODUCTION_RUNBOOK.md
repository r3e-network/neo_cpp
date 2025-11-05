# Neo C++ Production Deployment Runbook

## Table of Contents
1. [Pre-Deployment Checklist](#pre-deployment-checklist)
2. [System Requirements](#system-requirements)
3. [Security Configuration](#security-configuration)
4. [Deployment Steps](#deployment-steps)
5. [Post-Deployment Verification](#post-deployment-verification)
6. [Monitoring Setup](#monitoring-setup)
7. [Incident Response](#incident-response)
8. [Maintenance Procedures](#maintenance-procedures)
9. [Rollback Procedures](#rollback-procedures)

## Pre-Deployment Checklist

### Code Quality Checks
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] No critical security vulnerabilities in dependency scan
- [ ] Code coverage > 90%
- [ ] No hardcoded credentials or secrets
- [ ] All catch-all exception handlers replaced with specific handlers
- [ ] Input validation implemented for all external inputs

### Infrastructure Requirements
- [ ] Servers provisioned and configured
- [ ] Load balancers configured
- [ ] SSL certificates installed
- [ ] Firewall rules configured
- [ ] Backup systems in place
- [ ] Monitoring infrastructure ready

### Configuration
- [ ] Environment-specific configurations prepared
- [ ] Secrets management system configured
- [ ] Log aggregation configured
- [ ] Alert rules defined

## System Requirements

### Hardware Requirements

#### Minimum (TestNet/Development)
- CPU: 4 cores @ 2.5GHz
- RAM: 8GB
- Storage: 100GB SSD
- Network: 100 Mbps

#### Recommended (MainNet Production)
- CPU: 8+ cores @ 3.0GHz
- RAM: 32GB
- Storage: 500GB NVMe SSD
- Network: 1 Gbps
- Redundant power supply

### Software Requirements
- OS: Ubuntu 20.04 LTS or CentOS 8
- Compiler: GCC 9+ or Clang 10+
- Dependencies:
  - RocksDB 6.20+
  - OpenSSL 1.1.1+
  - Boost 1.70+
  - libcurl 7.68+

### Network Requirements
- Ports to open:
  - P2P: 10333 (MainNet), 20333 (TestNet)
  - RPC: 10332 (MainNet), 20332 (TestNet)
  - WebSocket: 10334 (MainNet), 20334 (TestNet)

## Security Configuration

### 1. System Hardening

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install security tools
sudo apt install -y fail2ban ufw unattended-upgrades

# Configure firewall
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow ssh
sudo ufw allow 10333/tcp  # P2P MainNet
sudo ufw allow 10332/tcp  # RPC MainNet (restrict source IPs in production)
sudo ufw enable

# Configure fail2ban
sudo systemctl enable fail2ban
sudo systemctl start fail2ban
```

### 2. User Configuration

```bash
# Create dedicated user
sudo useradd -m -s /bin/bash neonode
sudo usermod -aG sudo neonode

# Set up SSH key authentication
sudo -u neonode mkdir -p /home/neonode/.ssh
sudo -u neonode chmod 700 /home/neonode/.ssh
# Add your public key to /home/neonode/.ssh/authorized_keys

# Disable password authentication
sudo sed -i 's/PasswordAuthentication yes/PasswordAuthentication no/g' /etc/ssh/sshd_config
sudo systemctl restart sshd
```

### 3. RPC Security

Edit configuration to bind RPC to localhost only:
```json
{
  "ApplicationConfiguration": {
    "RPC": {
      "BindAddress": "127.0.0.1",
      "Port": 10332,
      "SslPort": 10331,
      "SslCert": "/path/to/cert.pem",
      "SslCertPassword": "${RPC_SSL_PASSWORD}",
      "DisabledMethods": [
        "sendrawtransaction",
        "submitblock"
      ]
    }
  }
}
```

Use nginx as reverse proxy with authentication:
```nginx
server {
    listen 443 ssl;
    server_name neo-rpc.example.com;
    
    ssl_certificate /etc/ssl/certs/neo-rpc.crt;
    ssl_certificate_key /etc/ssl/private/neo-rpc.key;
    
    location / {
        auth_basic "Neo RPC";
        auth_basic_user_file /etc/nginx/.htpasswd;
        
        proxy_pass http://127.0.0.1:10332;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        
        # Rate limiting
        limit_req zone=rpc_limit burst=10 nodelay;
    }
}
```

## Deployment Steps

### 1. Build the Application

```bash
# Clone repository
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp

# Create build directory
mkdir build && cd build

# Configure with production flags
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTS=OFF \
  -DENABLE_COVERAGE=OFF \
  -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Build
make -j$(nproc)

# Run tests one more time
ctest --output-on-failure

# Create deployment package
make package
```

### 2. Deploy Application

```bash
# Copy to production server
scp neo-cpp-*.tar.gz neonode@production-server:/home/neonode/

# On production server
ssh neonode@production-server

# Extract package
tar -xzf neo-cpp-*.tar.gz
cd neo-cpp-*

# Install
sudo make install

# Create directories
mkdir -p ~/neo-node/{logs,data,config}

# Copy configuration
cp /path/to/mainnet.json ~/neo-node/config/
```

### 3. Create Systemd Service

Create `/etc/systemd/system/neo-node.service`:
```ini
[Unit]
Description=Neo C++ Node
After=network.target

[Service]
Type=simple
User=neonode
Group=neonode
WorkingDirectory=/home/neonode/neo-node
ExecStart=/usr/local/bin/neo-cli --config /home/neonode/neo-node/config/mainnet.json
Restart=always
RestartSec=10

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/home/neonode/neo-node/data /home/neonode/neo-node/logs

# Resource limits
LimitNOFILE=65535
MemoryLimit=32G
CPUQuota=80%

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=neo-node

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable neo-node
sudo systemctl start neo-node
```

### ApplicationLogs plugin

- The `getapplicationlog` RPC requires the ApplicationLogs plugin. Verify it remains enabled in
  `PluginConfiguration.Plugins.ApplicationLogs` within your node configuration.
- The plugin uses an in-memory cache with a default size of 1,000 entries. Set `MaxCachedLogs` under the same
  configuration block to adjust retention and avoid unbounded growth if you expect heavy log volume.

## Post-Deployment Verification

### 1. Service Health Check

```bash
# Check service status
sudo systemctl status neo-node

# Check logs
sudo journalctl -u neo-node -f

# Verify process
ps aux | grep neo-cli

# Check ports
sudo netstat -tlnp | grep -E '10333|10332'
```

### 2. Functional Verification

```bash
# Check node version
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}'

# Check block count
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}'

# Check peers
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getpeers","params":[],"id":1}'
```

### 3. Performance Verification

```bash
# Monitor resource usage
htop

# Check disk I/O
iotop

# Monitor network connections
netstat -anp | grep -E '10333|10332' | wc -l

# Check sync progress
watch -n 5 'curl -s -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d "{\"jsonrpc\":\"2.0\",\"method\":\"getblockcount\",\"params\":[],\"id\":1}" \
  | jq -r .result'
```

## Monitoring Setup

### 1. Prometheus Metrics

Add to configuration:
```json
{
  "ApplicationConfiguration": {
    "Prometheus": {
      "Enabled": true,
      "Port": 9090
    }
  }
}
```

### 2. Key Metrics to Monitor

- **System Metrics**:
  - CPU usage < 80%
  - Memory usage < 90%
  - Disk usage < 85%
  - Disk I/O < 80% utilization

- **Application Metrics**:
  - Block height (should increase every ~15 seconds)
  - Peer count (should be > 3)
  - Memory pool size (< 50,000 transactions)
  - RPC response time (< 100ms for simple queries)

- **Network Metrics**:
  - Inbound connections
  - Outbound connections
  - Bandwidth usage
  - Packet loss < 0.1%

### 3. Alert Configuration

Critical alerts:
- Node not syncing for > 5 minutes
- Peer count < 3
- RPC not responding
- Disk space < 10GB
- Memory usage > 95%
- Too many failed RPC requests

## Incident Response

### Common Issues and Solutions

#### 1. Node Not Syncing

**Symptoms**: Block height not increasing

**Diagnosis**:
```bash
# Check peers
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getpeers","params":[],"id":1}'

# Check logs for errors
sudo journalctl -u neo-node --since "10 minutes ago" | grep ERROR
```

**Solutions**:
1. Check network connectivity
2. Verify firewall rules
3. Check if peers are banning the node
4. Restart with clean peer database

#### 2. High Memory Usage

**Symptoms**: Memory usage > 90%

**Diagnosis**:
```bash
# Check memory usage by component
pmap -x $(pgrep neo-cli) | tail -1

# Check for memory leaks
valgrind --leak-check=full /usr/local/bin/neo-cli
```

**Solutions**:
1. Increase memory limit
2. Reduce mempool size
3. Restart node
4. Check for memory leaks in custom plugins

#### 3. RPC Not Responding

**Symptoms**: RPC requests timeout

**Diagnosis**:
```bash
# Check if port is listening
sudo netstat -tlnp | grep 10332

# Check RPC thread status
gdb -p $(pgrep neo-cli) -ex "info threads" -ex "quit"
```

**Solutions**:
1. Check RPC configuration
2. Verify no deadlocks
3. Restart RPC service
4. Increase RPC worker threads

## Maintenance Procedures

### Daily Maintenance

1. **Log Rotation**:
```bash
# Configure logrotate
cat > /etc/logrotate.d/neo-node << EOF
/home/neonode/neo-node/logs/*.log {
    daily
    rotate 7
    compress
    missingok
    notifempty
    create 0644 neonode neonode
    postrotate
        systemctl reload neo-node
    endscript
}
EOF
```

2. **Backup Verification**:
```bash
# Verify latest backup
ls -la /backup/neo-node/
```

### Weekly Maintenance

1. **Security Updates**:
```bash
# Check for security updates
sudo unattended-upgrade --dry-run
```

2. **Performance Review**:
- Review monitoring dashboards
- Check for performance degradation
- Analyze slow query logs

### Monthly Maintenance

1. **Full Backup**:
```bash
# Stop node
sudo systemctl stop neo-node

# Create full backup
tar -czf /backup/neo-node-$(date +%Y%m%d).tar.gz \
  /home/neonode/neo-node/data

# Start node
sudo systemctl start neo-node
```

2. **Capacity Planning**:
- Review growth trends
- Plan for capacity upgrades
- Update resource allocations

## Rollback Procedures

### Preparation

1. **Before any upgrade**:
```bash
# Create restore point
sudo systemctl stop neo-node
cp -r /home/neonode/neo-node /home/neonode/neo-node.backup
sudo systemctl start neo-node
```

2. **Document current version**:
```bash
/usr/local/bin/neo-cli --version > /home/neonode/current-version.txt
```

### Rollback Steps

1. **Stop services**:
```bash
sudo systemctl stop neo-node
```

2. **Restore previous version**:
```bash
# Restore binaries
sudo cp /backup/neo-cli.previous /usr/local/bin/neo-cli

# Restore data if needed
rm -rf /home/neonode/neo-node/data
cp -r /home/neonode/neo-node.backup/data /home/neonode/neo-node/
```

3. **Verify configuration compatibility**:
```bash
/usr/local/bin/neo-cli --config /home/neonode/neo-node/config/mainnet.json --dry-run
```

4. **Start services**:
```bash
sudo systemctl start neo-node
```

5. **Verify functionality**:
```bash
# Check version
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}'

# Monitor logs
sudo journalctl -u neo-node -f
```

## Emergency Contacts

- On-call Engineer: [Phone/Slack]
- Team Lead: [Phone/Email]
- Infrastructure Team: [Slack Channel]
- Security Team: [Email]

## Appendix

### Useful Commands

```bash
# Get node statistics
neo-cli stats

# Export blocks
neo-cli export blocks 0 1000 /tmp/blocks.dat

# Verify blockchain integrity
neo-cli verify

# Clean mempool
neo-cli mempool clear

# Show configuration
neo-cli config show
```

### Performance Tuning

1. **Kernel Parameters** (`/etc/sysctl.conf`):
```
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 15
fs.file-max = 100000
vm.swappiness = 10
```

2. **RocksDB Tuning**:
```json
{
  "Storage": {
    "RocksDB": {
      "MaxOpenFiles": 5000,
      "WriteBufferSize": 67108864,
      "MaxWriteBufferNumber": 3,
      "TargetFileSizeBase": 67108864,
      "CompressionType": "lz4"
    }
  }
}
```

---

Last Updated: 2024-01-26
Version: 1.0.0
