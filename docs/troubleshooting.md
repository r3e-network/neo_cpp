# Neo C++ Troubleshooting Guide

## Overview

This guide provides solutions to common issues encountered when building, deploying, and running the Neo C++ blockchain node. Each section includes symptoms, root causes, and step-by-step solutions.

## Build Issues

### Compilation Errors

#### C++20 Standard Not Supported
**Symptoms:**
```
error: ISO C++20 does not allow comparison between pointer and zero-valued integral constant
error: 'concepts' header not found
```

**Causes:**
- Compiler version too old
- C++20 standard not enabled

**Solutions:**
```bash
# Update compiler (Ubuntu/Debian)
sudo apt update
sudo apt install gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 60

# Update compiler (CentOS/RHEL)
sudo dnf install gcc-toolset-11
scl enable gcc-toolset-11 bash

# Verify C++20 support
g++ --version
g++ -std=c++20 -E -x c++ - < /dev/null
```

#### Missing Dependencies
**Symptoms:**
```
CMake Error: Could not find a package configuration file provided by "Boost"
fatal error: openssl/ssl.h: No such file or directory
```

**Solutions:**
```bash
# Install development packages (Ubuntu/Debian)
sudo apt install -y \
    libboost-all-dev \
    libssl-dev \
    libleveldb-dev \
    libjson-c-dev \
    libgmp-dev \
    libsecp256k1-dev

# Install development packages (CentOS/RHEL)
sudo dnf install -y \
    boost-devel \
    openssl-devel \
    leveldb-devel \
    json-c-devel \
    gmp-devel \
    libsecp256k1-devel

# Verify installations
pkg-config --libs openssl
pkg-config --libs leveldb
```

#### vcpkg Package Issues
**Symptoms:**
```
CMake Error: Could not find a package configuration file
vcpkg integrate install failed
```

**Solutions:**
```bash
# Clean and rebuild vcpkg
cd vcpkg
git pull origin master
./bootstrap-vcpkg.sh
./vcpkg integrate install

# Clear package cache
./vcpkg remove --outdated
rm -rf buildtrees/ downloads/ packages/

# Install packages with triplet specification
./vcpkg install boost:x64-linux
./vcpkg install openssl:x64-linux
./vcpkg install leveldb:x64-linux

# Verify installation
./vcpkg list
```

#### CMake Configuration Issues
**Symptoms:**
```
CMake Error: The source directory does not appear to contain CMakeLists.txt
CMake Error: CMAKE_TOOLCHAIN_FILE not found
```

**Solutions:**
```bash
# Clean build directory
rm -rf build/
mkdir build && cd build

# Configure with proper toolchain
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20

# Alternative: Use preset if available
cmake --preset=default

# Debug CMake issues
cmake .. --debug-output
```

#### Linker Errors
**Symptoms:**
```
undefined reference to `boost::system::error_category::name() const'
ld: cannot find -lleveldb
```

**Solutions:**
```bash
# Check library paths
ldconfig -p | grep boost
ldconfig -p | grep leveldb

# Add library paths if needed
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Link with specific libraries
cmake .. -DCMAKE_EXE_LINKER_FLAGS="-L/usr/local/lib"

# Static linking option
cmake .. -DBUILD_SHARED_LIBS=OFF
```

### Memory Compilation Issues
**Symptoms:**
```
g++: internal compiler error: Killed (program cc1plus)
virtual memory exhausted: Cannot allocate memory
```

**Solutions:**
```bash
# Reduce parallel compilation
make -j2  # Instead of -j$(nproc)

# Add swap space
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# Check memory usage during compilation
htop  # Monitor in separate terminal

# Use distcc for distributed compilation
sudo apt install distcc
```

## Runtime Issues

### Node Startup Problems

#### Port Already in Use
**Symptoms:**
```
Error: bind: Address already in use
Failed to start P2P server on port 10333
```

**Solutions:**
```bash
# Find process using port
sudo netstat -tulpn | grep :10333
sudo lsof -i :10333

# Kill conflicting process
sudo kill -9 <PID>

# Use different port
./neo-cli --p2p-port 20333 --rpc-port 20332

# Check firewall settings
sudo ufw status
sudo iptables -L
```

#### Configuration File Issues
**Symptoms:**
```
Error: Failed to load protocol settings
Configuration file not found: protocol.json
```

**Solutions:**
```bash
# Create default configuration
mkdir -p ~/.neo
cat > ~/.neo/protocol.json << EOF
{
  "Network": 860833102,
  "AddressVersion": 53,
  "MillisecondsPerBlock": 15000,
  "MaxTransactionsPerBlock": 512,
  "SeedList": [
    "seed1.neo.org:10333",
    "seed2.neo.org:10333"
  ]
}
EOF

# Specify configuration path
./neo-cli --config /path/to/protocol.json

# Validate JSON syntax
python3 -m json.tool protocol.json
```

#### Database Corruption
**Symptoms:**
```
Error: Failed to open database
LevelDB: Corruption: bad block type
```

**Solutions:**
```bash
# Backup current database
cp -r ~/.neo/chain ~/.neo/chain.backup

# Attempt repair
./neo-cli --repair-database

# If repair fails, resync from genesis
rm -rf ~/.neo/chain
./neo-cli --resync

# Check disk space and integrity
df -h
fsck /dev/sda1
```

### Network Connectivity Issues

#### Peer Connection Problems
**Symptoms:**
```
Warning: No peers connected after 5 minutes
Error: Failed to connect to seed nodes
```

**Solutions:**
```bash
# Check internet connectivity
ping 8.8.8.8
ping seed1.neo.org

# Test specific ports
telnet seed1.neo.org 10333
nc -zv seed1.neo.org 10333

# Check firewall rules
sudo ufw allow 10333/tcp
sudo iptables -A INPUT -p tcp --dport 10333 -j ACCEPT

# Test with different seed nodes
./neo-cli --add-peer 1.2.3.4:10333

# Check NAT/router configuration
# Forward port 10333 in router settings
```

#### Slow Block Synchronization
**Symptoms:**
```
Info: Block height: 1000/5000000 (0.02%)
Warning: Sync speed below 10 blocks/second
```

**Solutions:**
```bash
# Check disk I/O performance
iostat -x 1

# Optimize database settings
cat > ~/.neo/storage.json << EOF
{
  "Engine": "LevelDB",
  "Options": {
    "WriteBufferSize": 67108864,
    "MaxOpenFiles": 2000,
    "BlockSize": 65536
  }
}
EOF

# Use SSD storage
# Move chain data to SSD partition

# Increase peer connections
./neo-cli --max-peers 50

# Fast sync from trusted source
./neo-cli --fast-sync --trusted-peer trusted.node.com:10333
```

### Memory and Performance Issues

#### High Memory Usage
**Symptoms:**
```
Warning: Memory usage above 8GB
System: Out of memory, killing process
```

**Solutions:**
```bash
# Monitor memory usage
htop
ps aux --sort=-%mem | head

# Reduce cache sizes
cat > ~/.neo/performance.json << EOF
{
  "BlockCacheSize": 268435456,
  "MemPoolMaxTransactions": 10000,
  "MaxConcurrentConnections": 20
}
EOF

# Enable memory monitoring
./neo-cli --memory-limit 4G

# Check for memory leaks
valgrind --tool=memcheck --leak-check=full ./neo-cli

# Restart node periodically (systemd timer)
```

#### High CPU Usage
**Symptoms:**
```
System: CPU usage consistently above 90%
Warning: Block processing time exceeding targets
```

**Solutions:**
```bash
# Check CPU usage per thread
top -H -p $(pgrep neo-cli)

# Reduce thread count
./neo-cli --worker-threads 4

# Lower process priority
nice -n 10 ./neo-cli

# Optimize compilation flags
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Profile CPU usage
perf record -g ./neo-cli
perf report
```

### Consensus Issues

#### Validator Node Problems
**Symptoms:**
```
Error: Failed to create consensus message
Warning: View change timeout exceeded
```

**Solutions:**
```bash
# Check validator key configuration
./neo-cli --check-validator-key

# Verify time synchronization
sudo ntpdate -s time.nist.gov
timedatectl status

# Check network latency to other validators
ping -c 10 validator2.neo.org
traceroute validator2.neo.org

# Monitor consensus state
./neo-cli --consensus-debug

# Restart consensus service
./neo-cli --restart-consensus
```

#### Block Creation Failures
**Symptoms:**
```
Error: Block validation failed
Warning: Consensus timeout, proposing empty block
```

**Solutions:**
```bash
# Check transaction pool
./neo-cli rpc gettransaction

# Verify gas calculations
./neo-cli --verify-gas-calculations

# Check storage consistency
./neo-cli --verify-storage

# Reset consensus state
./neo-cli --reset-consensus-state

# Check for corrupted transactions
./neo-cli --validate-mempool
```

## RPC Service Issues

### JSON-RPC Errors

#### Connection Refused
**Symptoms:**
```
curl: (7) Failed to connect to localhost port 10332: Connection refused
```

**Solutions:**
```bash
# Check if RPC service is enabled
./neo-cli --enable-rpc

# Verify binding address
./neo-cli --rpc-bind 0.0.0.0 --rpc-port 10332

# Check service status
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}'

# Check firewall for RPC port
sudo ufw allow 10332/tcp

# Test with different binding
./neo-cli --rpc-bind 127.0.0.1
```

#### Invalid JSON Response
**Symptoms:**
```
{"error":{"code":-32700,"message":"Parse error"}}
{"error":{"code":-32601,"message":"Method not found"}}
```

**Solutions:**
```bash
# Validate JSON request format
echo '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}' | python3 -m json.tool

# Check available methods
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"listmethods","params":[],"id":1}'

# Enable additional RPC methods
./neo-cli --enable-rpc-methods invokefunction,invokescript

# Check RPC logs
tail -f ~/.neo/logs/rpc.log
```

#### Slow RPC Response
**Symptoms:**
```
Warning: RPC request timeout
Error: Request took longer than 30 seconds
```

**Solutions:**
```bash
# Increase RPC timeout
./neo-cli --rpc-timeout 60

# Check database performance
iostat -x 1

# Optimize RPC settings
cat > ~/.neo/rpc.json << EOF
{
  "MaxConnections": 100,
  "Timeout": 60,
  "KeepAlive": true,
  "ThreadPool": 10
}
EOF

# Monitor RPC performance
./neo-cli --rpc-metrics

# Use connection pooling
# Configure client to reuse connections
```

## Smart Contract Issues

### VM Execution Errors

#### Script Execution Failures
**Symptoms:**
```
VM State: FAULT
Error: Unhandled exception in smart contract
```

**Solutions:**
```bash
# Debug script execution
./neo-cli invoke-debug <script_hash> <method> <params>

# Check gas limits
./neo-cli invoke-function <script_hash> <method> <params> --gas 10

# Validate script bytecode
./neo-cli validate-script <script_hex>

# Check contract state
./neo-cli rpc getcontractstate <script_hash>

# Review execution logs
tail -f ~/.neo/logs/vm.log
```

#### Storage Access Errors
**Symptoms:**
```
Error: Storage key not found
RuntimeError: Unauthorized storage access
```

**Solutions:**
```bash
# Check storage permissions
./neo-cli rpc getstorage <script_hash> <key>

# Verify contract deployment
./neo-cli rpc getcontractstate <script_hash>

# Check storage consistency
./neo-cli --verify-storage <script_hash>

# Debug storage operations
./neo-cli --trace-storage-access

# Reset storage if corrupted
./neo-cli --reset-contract-storage <script_hash>
```

### Contract Deployment Issues

#### Deployment Failures
**Symptoms:**
```
Error: Contract deployment failed
Transaction rejected: Invalid manifest
```

**Solutions:**
```bash
# Validate contract manifest
python3 -m json.tool manifest.json

# Check deployment transaction
./neo-cli rpc getrawtransaction <deployment_tx>

# Verify contract bytecode
./neo-cli validate-nef <contract.nef>

# Check gas fees
./neo-cli estimate-gas-deployment <contract.nef> <manifest.json>

# Review deployment logs
grep "deployment" ~/.neo/logs/node.log
```

## Logging and Debugging

### Enable Debug Logging
```bash
# Increase log verbosity
./neo-cli --log-level debug

# Enable specific module logging
./neo-cli --debug-modules consensus,network,vm

# Real-time log monitoring
tail -f ~/.neo/logs/node.log | grep ERROR

# Log rotation configuration
cat > ~/.neo/log.json << EOF
{
  "Level": "debug",
  "File": "~/.neo/logs/node.log",
  "MaxSize": "100MB",
  "MaxBackups": 10,
  "Compress": true
}
EOF
```

### Performance Profiling
```bash
# CPU profiling
perf record -g ./neo-cli
perf report

# Memory profiling
valgrind --tool=massif ./neo-cli
ms_print massif.out.*

# Network profiling
tcpdump -i eth0 port 10333 -w network.pcap
wireshark network.pcap

# I/O profiling
iotop -p $(pgrep neo-cli)
```

### Core Dump Analysis
```bash
# Enable core dumps
ulimit -c unlimited
echo 'core.%e.%p' | sudo tee /proc/sys/kernel/core_pattern

# Analyze core dump
gdb ./neo-cli core.neo-cli.12345
(gdb) bt
(gdb) thread apply all bt
(gdb) info registers
```

## System Administration

### Service Management

#### Systemd Service Issues
**Symptoms:**
```
Job for neo-node.service failed
neo-node.service: Start request repeated too quickly
```

**Solutions:**
```bash
# Check service status
systemctl status neo-node.service

# View service logs
journalctl -u neo-node.service -f

# Reset failed state
systemctl reset-failed neo-node.service

# Restart with delay
systemctl edit neo-node.service
# Add:
# [Service]
# RestartSec=30

# Check service configuration
systemctl cat neo-node.service
```

#### Permission Issues
**Symptoms:**
```
Permission denied: /opt/neo/data
failed to create directory: Operation not permitted
```

**Solutions:**
```bash
# Fix ownership
sudo chown -R neo:neo /opt/neo

# Fix permissions
sudo chmod 755 /opt/neo
sudo chmod 750 /opt/neo/data
sudo chmod 600 /opt/neo/config/*.json

# Check SELinux context (if applicable)
ls -Z /opt/neo
sudo setsebool -P httpd_can_network_connect 1

# Create systemd user
sudo useradd -r -s /bin/false neo
sudo usermod -aG neo $USER
```

### Backup and Recovery

#### Backup Corruption
**Symptoms:**
```
Error: Cannot restore from backup
Backup file appears corrupted
```

**Solutions:**
```bash
# Verify backup integrity
tar -tzf backup.tar.gz > /dev/null

# Test restore in temporary location
mkdir /tmp/test-restore
tar -xzf backup.tar.gz -C /tmp/test-restore

# Use checksums for verification
sha256sum backup.tar.gz
sha256sum -c backup.sha256

# Restore from multiple backup sources
# Keep daily, weekly, monthly backups
```

#### Fast Recovery Procedures
```bash
# Quick sync from snapshot
wget https://snapshots.neo.org/latest.tar.gz
tar -xzf latest.tar.gz -C ~/.neo/

# Bootstrap from trusted peer
./neo-cli --bootstrap-from trusted-node.neo.org:10333

# Parallel sync (if available)
./neo-cli --parallel-sync --sync-threads 4
```

## Emergency Procedures

### Node Recovery Checklist
1. **Stop all services**
   ```bash
   systemctl stop neo-node
   ```

2. **Create emergency backup**
   ```bash
   tar -czf emergency-backup-$(date +%Y%m%d-%H%M%S).tar.gz ~/.neo/data
   ```

3. **Check system resources**
   ```bash
   df -h
   free -h
   top
   ```

4. **Verify configuration**
   ```bash
   ./neo-cli --config-test
   ```

5. **Start in safe mode**
   ```bash
   ./neo-cli --safe-mode --no-consensus
   ```

6. **Monitor startup**
   ```bash
   tail -f ~/.neo/logs/node.log
   ```

### Network Split Recovery
```bash
# Detect network split
./neo-cli network-status

# Check consensus state
./neo-cli consensus-info

# Manual intervention if needed
./neo-cli reset-consensus
./neo-cli resync-from-genesis

# Coordinate with other validators
# Contact network administrators
```

### Data Recovery Tools
```bash
# Database repair utility
./tools/db-repair --database ~/.neo/data/chain

# Transaction validation tool
./tools/validate-chain --start-block 0 --end-block latest

# Storage consistency checker
./tools/check-storage --fix-errors

# Block validation utility
./tools/validate-blocks --parallel --fix-minor-errors
```

This troubleshooting guide provides comprehensive solutions for most common issues encountered when operating a Neo C++ blockchain node. For issues not covered here, consult the development team or community support channels.