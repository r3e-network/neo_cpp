# Neo N3 C++ Node - Deployment Guide

## Overview

This guide provides comprehensive instructions for building, configuring, and deploying the Neo N3 C++ node implementation.

## Prerequisites

### System Requirements
- **Operating System**: Windows 10+, Ubuntu 20.04+, macOS 11+
- **RAM**: Minimum 4GB, Recommended 8GB+
- **Storage**: Minimum 50GB free space for blockchain data
- **Network**: Stable internet connection for P2P networking

### Development Tools
- **C++ Compiler**: 
  - GCC 10+ (Linux)
  - Clang 12+ (macOS)
  - MSVC 2019+ (Windows)
- **CMake**: Version 3.16 or higher
- **Git**: For source code management

### Dependencies
- **OpenSSL**: Cryptographic library (1.1.1+)
- **nlohmann/json**: JSON parsing library
- **GoogleTest**: Unit testing framework (automatically downloaded)
- **blst** (Optional): For BLS12_381 cryptography support

## Building from Source

### 1. Clone the Repository
```bash
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp
```

### 2. Install Dependencies

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libssl-dev nlohmann-json3-dev
```

#### macOS (with Homebrew)
```bash
brew install cmake openssl nlohmann-json
```

#### Windows (with vcpkg)
```bash
vcpkg install openssl nlohmann-json
```

### 3. Configure Build
```bash
mkdir build && cd build

# Basic configuration
cmake .. -DCMAKE_BUILD_TYPE=Release

# With optional BLS12_381 support
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_BLS12_381=ON

# Windows with vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg.cmake
```

### 4. Build the Project
```bash
# Build all targets
cmake --build . --config Release

# Build with parallel jobs
cmake --build . --config Release -j$(nproc)
```

### 5. Run Tests
```bash
# Run all unit tests
ctest --output-on-failure

# Run specific test suites
ctest -R "VM|Crypto|IO" --output-on-failure
```

## Configuration

### Node Configuration File
Create `config.json` in the node directory:

```json
{
  "ApplicationConfiguration": {
    "Logger": {
      "Path": "Logs",
      "ConsoleOutput": true,
      "Active": true
    },
    "Storage": {
      "Engine": "LevelDBStore",
      "Path": "Data_LevelDB_{0}"
    },
    "P2P": {
      "Port": 10333,
      "WsPort": 10334,
      "MinDesiredConnections": 10,
      "MaxConnections": 40,
      "MaxConnectionsPerAddress": 3
    },
    "RPC": {
      "BindAddress": "127.0.0.1",
      "Port": 10332,
      "SslCert": "",
      "SslCertPassword": "",
      "TrustedAuthorities": [],
      "MaxGasInvoke": 50000000,
      "MaxIteratorResultItems": 100,
      "SessionEnabled": false,
      "SessionExpirationTime": 60,
      "MaxStackSize": 65536
    }
  },
  "ProtocolConfiguration": {
    "Network": 860833102,
    "AddressVersion": 53,
    "MillisecondsPerBlock": 15000,
    "MaxTransactionsPerBlock": 512,
    "MemoryPoolMaxTransactions": 50000,
    "MaxTraceableBlocks": 2102400,
    "InitialGasDistribution": 5200000000000000,
    "ValidatorsCount": 7,
    "StandbyCommittee": [
      "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
      "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
      "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a",
      "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
      "024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d",
      "02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e",
      "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70"
    ],
    "SeedList": [
      "seed1.neo.org:10333",
      "seed2.neo.org:10333",
      "seed3.neo.org:10333",
      "seed4.neo.org:10333",
      "seed5.neo.org:10333"
    ]
  }
}
```

### Environment Variables
```bash
# Set data directory
export NEO_DATA_DIR=/path/to/blockchain/data

# Set log level
export NEO_LOG_LEVEL=INFO

# Set network (mainnet/testnet)
export NEO_NETWORK=mainnet
```

## Running the Node

### 1. Start the Node
```bash
# Basic startup
./neo-node

# With custom config
./neo-node --config=/path/to/config.json

# With specific data directory
./neo-node --datadir=/path/to/data

# Enable RPC server
./neo-node --rpc
```

### 2. Command Line Options
```bash
./neo-node [OPTIONS]

Options:
  --config=FILE         Configuration file path
  --datadir=DIR         Data directory path
  --rpc                 Enable RPC server
  --rpc-port=PORT       RPC server port (default: 10332)
  --p2p-port=PORT       P2P network port (default: 10333)
  --testnet             Connect to testnet
  --help                Show help message
  --version             Show version information
```

### 3. Interactive Console
Once the node is running, you can use the interactive console:

```bash
# Show help
neo> help

# Get blockchain info
neo> show state

# Get block count
neo> show pool

# Create wallet
neo> create wallet wallet.json

# Open wallet
neo> open wallet wallet.json
```

## Monitoring and Maintenance

### Health Checks
```bash
# Check node status via RPC
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}'

# Check block height
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}'
```

### Log Management
```bash
# View real-time logs
tail -f Logs/neo.log

# Rotate logs (if using logrotate)
sudo logrotate /etc/logrotate.d/neo-node
```

### Performance Monitoring
```bash
# Monitor resource usage
htop
iostat -x 1
netstat -an | grep :10333

# Check database size
du -sh Data_LevelDB_*
```

## Security Considerations

### Network Security
- Use firewall to restrict access to RPC port (10332)
- Only expose P2P port (10333) to the internet
- Consider using VPN for remote access

### Wallet Security
- Store wallet files in encrypted storage
- Use strong passwords for wallet encryption
- Backup wallet files securely
- Never share private keys

### System Security
- Keep system and dependencies updated
- Use dedicated user account for node process
- Enable system logging and monitoring
- Regular security audits

## Troubleshooting

### Common Issues

#### Build Errors
```bash
# Missing dependencies
sudo apt install build-essential cmake libssl-dev

# CMake version too old
# Install newer CMake from official website
```

#### Runtime Errors
```bash
# Port already in use
netstat -tulpn | grep :10333
sudo kill -9 <PID>

# Permission denied
sudo chown -R $USER:$USER /path/to/neo/data
chmod 755 /path/to/neo/data
```

#### Synchronization Issues
```bash
# Check network connectivity
ping seed1.neo.org

# Verify configuration
cat config.json | jq .ProtocolConfiguration.SeedList

# Reset blockchain data (if corrupted)
rm -rf Data_LevelDB_*
```

### Getting Help
- **Documentation**: Check @docs/ directory
- **Issues**: Report bugs on GitHub
- **Community**: Join Neo developer Discord
- **Support**: Contact Neo development team

## Production Deployment

### Systemd Service (Linux)
Create `/etc/systemd/system/neo-node.service`:

```ini
[Unit]
Description=Neo N3 Node
After=network.target

[Service]
Type=simple
User=neo
WorkingDirectory=/opt/neo
ExecStart=/opt/neo/neo-node --config=/opt/neo/config.json
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable neo-node
sudo systemctl start neo-node
sudo systemctl status neo-node
```

### Docker Deployment
```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    build-essential cmake libssl-dev nlohmann-json3-dev

COPY . /neo
WORKDIR /neo

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --config Release

EXPOSE 10332 10333

CMD ["./build/neo-node"]
```

### Load Balancer Configuration
For RPC load balancing, configure nginx:

```nginx
upstream neo_rpc {
    server 127.0.0.1:10332;
    server 127.0.0.1:10342;
    server 127.0.0.1:10352;
}

server {
    listen 80;
    location / {
        proxy_pass http://neo_rpc;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

This completes the comprehensive deployment guide for the Neo N3 C++ implementation.
