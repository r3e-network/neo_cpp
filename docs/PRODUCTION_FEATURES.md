# Neo C++ Production Features

## Overview

The Neo C++ implementation includes comprehensive production features for enterprise deployment. This document details all production-ready components and their usage.

## Table of Contents
1. [Configuration Management](#configuration-management)
2. [Monitoring & Metrics](#monitoring--metrics)
3. [Health Checks](#health-checks)
4. [Rate Limiting](#rate-limiting)
5. [Connection Management](#connection-management)
6. [Circuit Breakers](#circuit-breakers)
7. [Graceful Shutdown](#graceful-shutdown)
8. [Error Handling](#error-handling)
9. [Security Features](#security-features)

## Configuration Management

### Environment Variable Support

The configuration system supports environment variable substitution and overrides:

```json
{
  "ApplicationConfiguration": {
    "RPC": {
      "Port": "${NEO_RPC_PORT}",
      "BindAddress": "${NEO_RPC_BIND_ADDRESS}",
      "SslCert": "${NEO_SSL_CERT_PATH}",
      "SslCertPassword": "${NEO_SSL_CERT_PASSWORD}"
    }
  }
}
```

### Environment Variable Naming

Configuration paths are automatically converted to environment variables:
- `ApplicationConfiguration.RPC.Port` → `NEO_APPLICATIONCONFIGURATION_RPC_PORT`
- `ProtocolConfiguration.Magic` → `NEO_PROTOCOLCONFIGURATION_MAGIC`

### Configuration Hierarchy

1. **Environment Variables** (highest priority)
2. **Configuration File**
3. **Default Values** (lowest priority)

### Usage Example

```cpp
#include <neo/core/config_manager.h>

// Load configuration
auto& config = neo::core::ConfigManager::GetInstance();
config.LoadFromFile("config.json");

// Access values
uint16_t port = config.GetPort("ApplicationConfiguration.RPC.Port", 10332);
std::string bindAddr = config.GetString("ApplicationConfiguration.RPC.BindAddress", "127.0.0.1");
bool enabled = config.GetBool("ApplicationConfiguration.RPC.Enabled", true);
```

## Monitoring & Metrics

### Prometheus Integration

Built-in Prometheus metrics exporter with standard and custom metrics:

```cpp
#include <neo/monitoring/prometheus_exporter.h>

// Create metrics
auto counter = PROMETHEUS_COUNTER("requests_total", "Total requests");
auto gauge = PROMETHEUS_GAUGE("active_connections", "Active connections");
auto histogram = PROMETHEUS_HISTOGRAM("request_duration_seconds", "Request duration");

// Use metrics
counter->Increment();
gauge->Set(42);
histogram->Observe(0.123);

// Start metrics server
auto& exporter = neo::monitoring::PrometheusExporter::GetInstance();
exporter.StartServer(9090); // http://localhost:9090/metrics
```

### Available Metrics

#### System Metrics
- `neo_cpu_usage_percent` - CPU usage percentage
- `neo_memory_usage_bytes` - Memory usage in bytes
- `neo_disk_usage_bytes` - Disk usage in bytes
- `process_uptime_seconds` - Process uptime

#### Blockchain Metrics
- `neo_block_height` - Current blockchain height
- `neo_header_height` - Current header height
- `neo_block_processing_seconds` - Block processing time histogram
- `neo_transactions_processed_total` - Total transactions processed
- `neo_transactions_failed_total` - Total transaction failures

#### Network Metrics
- `neo_peer_count` - Number of connected peers
- `neo_bytes_received_total` - Total bytes received
- `neo_bytes_sent_total` - Total bytes sent
- `neo_connections_active` - Active connections

#### RPC Metrics
- `neo_rpc_requests_total{method}` - RPC requests by method
- `neo_rpc_latency_seconds{method}` - RPC latency by method
- `neo_rpc_errors_total{method,error}` - RPC errors by method and type

## Health Checks

### Built-in Health Checks

1. **Blockchain Health** - Sync status and block processing
2. **P2P Health** - Peer connectivity and network status
3. **Memory Health** - Memory usage monitoring
4. **RPC Health** - RPC server status
5. **Storage Health** - Database connectivity

### Health Check Endpoint

```bash
# Get overall health status
curl http://localhost:9090/health

{
  "status": "healthy",
  "timestamp": 1640995200,
  "checks": [
    {
      "name": "blockchain",
      "status": "healthy",
      "message": "Blockchain synced",
      "responseTime": 5,
      "details": {
        "height": "1234567",
        "headerHeight": "1234567"
      }
    }
  ]
}
```

### Custom Health Checks

```cpp
#include <neo/monitoring/health_check.h>

class CustomHealthCheck : public neo::monitoring::HealthCheck {
public:
    CustomHealthCheck() : HealthCheck("custom") {}
    
    neo::monitoring::HealthCheckResult Check() override {
        neo::monitoring::HealthCheckResult result;
        result.name = name_;
        result.timestamp = std::chrono::system_clock::now();
        
        // Perform your check
        if (/* condition */) {
            result.status = neo::monitoring::HealthStatus::HEALTHY;
            result.message = "Service operational";
        } else {
            result.status = neo::monitoring::HealthStatus::UNHEALTHY;
            result.message = "Service down";
        }
        
        return result;
    }
};

// Register
auto& manager = neo::monitoring::HealthCheckManager::GetInstance();
manager.RegisterHealthCheck(std::make_shared<CustomHealthCheck>());
```

## Rate Limiting

### Method-Specific Rate Limiting

Built-in rate limiting with different limits per RPC method:

```cpp
#include <neo/rpc/rate_limiter.h>

// Configure rate limits
neo::rpc::MethodRateLimiter limiter;
limiter.SetMethodLimit("sendrawtransaction", {1, 10, true});    // 1/sec, 10/min
limiter.SetMethodLimit("invokefunction", {5, 100, true});       // 5/sec, 100/min
limiter.SetMethodLimit("getblock", {10, 300, true});            // 10/sec, 300/min

// Check if request is allowed
if (limiter.IsAllowed(clientIP, "sendrawtransaction")) {
    // Process request
} else {
    // Reject with 429 Too Many Requests
}
```

### Token Bucket Algorithm

Uses token bucket algorithm with configurable:
- Requests per second
- Requests per minute  
- Burst capacity
- Per-client limits

## Connection Management

### Connection Pooling

Efficient connection pooling for external services:

```cpp
#include <neo/network/connection_manager.h>

// Configure connection pool
neo::network::ConnectionPool<HttpConnection>::Config config;
config.minConnections = 5;
config.maxConnections = 50;
config.connectionTimeout = std::chrono::seconds(30);
config.idleTimeout = std::chrono::minutes(5);

// Create pool
auto factory = []() { return std::make_shared<HttpConnection>(); };
auto validator = [](auto conn) { return conn->IsValid(); };
neo::network::ConnectionPool<HttpConnection> pool(config, factory, validator);

// Use connection
auto conn = pool.Acquire();
if (conn) {
    // Use connection
    pool.Release(conn);
}
```

### Connection Limits

Per-IP and total connection limits:

```cpp
#include <neo/network/connection_manager.h>

neo::network::ConnectionLimits::Config config;
config.maxConnectionsPerIP = 5;
config.maxTotalConnections = 1000;
config.connectionRateWindow = std::chrono::minutes(1);
config.maxConnectionsPerWindow = 100;

neo::network::ConnectionLimits limits(config);

// Check if connection allowed
if (limits.IsConnectionAllowed(clientIP)) {
    limits.RegisterConnection(clientIP);
    // Accept connection
} else {
    // Reject connection
}
```

### Timeout Management

Centralized timeout management:

```cpp
#include <neo/network/connection_manager.h>

neo::network::TimeoutManager timeouts;
timeouts.Start();

// Schedule timeout
auto id = timeouts.Schedule(std::chrono::seconds(30), []() {
    // Timeout callback
    std::cout << "Operation timed out" << std::endl;
});

// Cancel if completed early
timeouts.Cancel(id);
```

## Circuit Breakers

### Fault Tolerance

Circuit breaker pattern for external service calls:

```cpp
#include <neo/core/circuit_breaker.h>

// Configure circuit breaker
neo::core::CircuitBreaker::Config config;
config.failureThreshold = 5;           // Open after 5 failures
config.failureRateThreshold = 0.5;     // Or 50% failure rate
config.timeout = std::chrono::minutes(1); // Stay open for 1 minute
config.successThreshold = 3;           // Close after 3 successes

neo::core::CircuitBreaker breaker("external_service", config);

// Use circuit breaker
try {
    auto result = breaker.Execute([]() {
        // Call external service
        return CallExternalAPI();
    });
} catch (const std::runtime_error& e) {
    // Circuit is open, use fallback
    return GetCachedData();
}
```

### With Fallback

```cpp
auto result = breaker.ExecuteWithFallback(
    []() { return CallExternalAPI(); },    // Primary function
    []() { return GetCachedData(); }       // Fallback function
);
```

### Circuit Breaker States

1. **CLOSED** - Normal operation, requests pass through
2. **OPEN** - Failures exceeded threshold, requests fail fast
3. **HALF_OPEN** - Testing if service recovered

## Graceful Shutdown

### Shutdown Manager

Coordinates graceful shutdown with prioritized handlers:

```cpp
#include <neo/core/shutdown_manager.h>

auto& manager = neo::core::ShutdownManager::GetInstance();

// Install signal handlers
manager.InstallSignalHandlers();

// Register shutdown handlers
manager.RegisterHandler("database", []() {
    database.flush();
    database.close();
}, 10, std::chrono::seconds(30)); // Priority 10, 30s timeout

manager.RegisterHandler("network", []() {
    server.stop();
}, 20, std::chrono::seconds(10)); // Priority 20, 10s timeout

// Wait for shutdown
manager.WaitForShutdown();
```

### Shutdown Sequence

Default Neo CLI shutdown sequence:

1. **Priority 10**: Stop accepting new connections
2. **Priority 20**: Close wallet
3. **Priority 30**: Stop consensus
4. **Priority 40**: Flush memory pool
5. **Priority 50**: Stop P2P server
6. **Priority 60**: Stop RPC server
7. **Priority 70**: Stop monitoring
8. **Priority 80**: Flush and close storage
9. **Priority 90**: Final cleanup

### Macro for Easy Registration

```cpp
REGISTER_SHUTDOWN_HANDLER(my_service, 50, 30000, {
    myService.stop();
    myService.cleanup();
});
```

## Error Handling

### Safe Conversions

Input validation with comprehensive error handling:

```cpp
#include <neo/core/safe_conversions.h>

try {
    int32_t value = neo::core::SafeConversions::SafeToInt32("123");
    uint16_t port = neo::core::SafeConversions::SafeToPort("8080");
    uint64_t amount = neo::core::SafeConversions::SafeToUInt64("1000000");
} catch (const std::runtime_error& e) {
    // Handle conversion error
    std::cerr << "Invalid input: " << e.what() << std::endl;
}

// Optional versions
auto maybeValue = neo::core::SafeConversions::TryToInt32("invalid");
if (maybeValue) {
    // Use value
    int32_t value = *maybeValue;
}
```

### RPC Validation

Comprehensive RPC parameter validation:

```cpp
#include <neo/rpc/rpc_validation.h>

// Validate parameters
neo::rpc::RpcValidation::ValidateParamCount(params, 1, 3);
std::string hash = neo::rpc::RpcValidation::GetStringParam(params, 0, "hash");
neo::rpc::RpcValidation::ValidateBlockHash(hash);

// Type-safe parameter extraction
int64_t height = neo::rpc::RpcValidation::GetIntParam(params, 1, "height");
bool verbose = neo::rpc::RpcValidation::GetBoolParam(params, 2, "verbose");
```

## Security Features

### Input Sanitization

All external inputs are validated and sanitized:

- Hex string validation with length checking
- Address format validation with Base58 checking
- Port range validation (1-65535)
- Gas amount validation with overflow protection
- Script size limits (max 1MB)

### Rate Limiting

Multiple layers of rate limiting:

- Global rate limits (requests per second/minute)
- Per-method rate limits
- Per-client IP limits
- Burst capacity controls

### Connection Security

Network security features:

- Maximum connections per IP
- Connection rate limiting
- Timeout management
- Graceful connection handling

### Configuration Security

Secure configuration handling:

- Environment variable support for secrets
- No hardcoded credentials
- Secure defaults
- Input validation

## Production Deployment

### Environment Variables

Set these environment variables for production:

```bash
# Network configuration
export NEO_APPLICATIONCONFIGURATION_RPC_BINDADDRESS="127.0.0.1"
export NEO_APPLICATIONCONFIGURATION_RPC_PORT="10332"

# Security
export NEO_SSL_CERT_PATH="/etc/ssl/certs/neo.crt"
export NEO_SSL_CERT_PASSWORD="your_cert_password"

# Database
export NEO_APPLICATIONCONFIGURATION_STORAGE_PATH="/data/neo/chain"

# Monitoring
export NEO_APPLICATIONCONFIGURATION_PROMETHEUS_PORT="9090"
```

### Production Configuration

Example production configuration:

```json
{
  "ApplicationConfiguration": {
    "Logger": {
      "Path": "/var/log/neo",
      "ConsoleOutput": false,
      "Active": true
    },
    "Storage": {
      "Engine": "RocksDBStore",
      "Path": "${NEO_DATA_PATH}"
    },
    "P2P": {
      "Port": 10333,
      "MaxConnections": 100,
      "MaxConnectionsPerAddress": 5
    },
    "RPC": {
      "Port": 10332,
      "BindAddress": "${NEO_RPC_BIND_ADDRESS}",
      "SslPort": 10331,
      "SslCert": "${NEO_SSL_CERT_PATH}",
      "SslCertPassword": "${NEO_SSL_CERT_PASSWORD}",
      "MaxGasInvoke": "50"
    },
    "Prometheus": {
      "Port": 9090,
      "Enabled": true
    }
  },
  "ProtocolConfiguration": {
    "Magic": 860833102,
    "MemoryPoolMaxTransactions": 50000
  }
}
```

### Service Unit File

```ini
[Unit]
Description=Neo C++ Node
After=network.target

[Service]
Type=simple
User=neo
Group=neo
WorkingDirectory=/opt/neo
ExecStart=/usr/local/bin/neo-cli --config /etc/neo/mainnet.json
Restart=always
RestartSec=10

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true

# Resource limits
LimitNOFILE=65535
MemoryLimit=32G

# Environment
Environment=NEO_RPC_BIND_ADDRESS=127.0.0.1
Environment=NEO_DATA_PATH=/var/lib/neo

[Install]
WantedBy=multi-user.target
```

### Monitoring Setup

Configure Prometheus to scrape metrics:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'neo-node'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: /metrics
    scrape_interval: 15s
```

Configure alerts:

```yaml
# neo-alerts.yml
groups:
  - name: neo
    rules:
      - alert: NeoNodeDown
        expr: up{job="neo-node"} == 0
        for: 1m
        annotations:
          summary: "Neo node is down"
          
      - alert: NeoPeerCountLow
        expr: neo_peer_count < 3
        for: 5m
        annotations:
          summary: "Neo node has insufficient peers"
          
      - alert: NeoSyncBehind
        expr: neo_header_height - neo_block_height > 100
        for: 10m
        annotations:
          summary: "Neo node is behind on synchronization"
```

---

Last Updated: 2024-01-26
Version: 1.0.0