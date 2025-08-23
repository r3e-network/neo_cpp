# Production Security Guide

This document outlines security measures implemented in the Neo C++ node for production deployment.

## Configuration Security

### Environment Variables
Production-sensitive configuration should be provided via environment variables rather than configuration files:

```bash
# Database configuration
export NEO_DB_PATH="/var/lib/neo-cpp/data"
export NEO_DB_CACHE_SIZE_MB=1024

# Network configuration  
export NEO_NETWORK_P2P_PORT=10333
export NEO_NETWORK_BIND_ADDRESS="0.0.0.0"

# RPC configuration
export NEO_RPC_PORT=10332
export NEO_RPC_BIND_ADDRESS="127.0.0.1"
export NEO_RPC_ENABLE_AUTH=true
export NEO_RPC_USERNAME="admin"
export NEO_RPC_PASSWORD="$(openssl rand -base64 32)"

# Consensus configuration (if participating in consensus)
export NEO_CONSENSUS_ENABLED=false
export NEO_CONSENSUS_WALLET_PATH="/secure/path/to/wallet.json"
export NEO_CONSENSUS_WALLET_PASSWORD="$(cat /secure/path/to/wallet.password)"

# Logging configuration
export NEO_LOG_LEVEL=info
export NEO_LOG_FILE_PATH="/var/log/neo-cpp/neo.log"

# Monitoring configuration
export NEO_METRICS_ENABLED=true
export NEO_METRICS_PORT=9090
export NEO_HEALTH_CHECK_PORT=8080
```

### File Permissions
Ensure proper file permissions for sensitive files:

```bash
# Configuration files
chmod 600 /etc/neo-cpp/config.json
chown neo:neo /etc/neo-cpp/config.json

# Wallet files
chmod 600 /secure/path/to/wallet.json
chown neo:neo /secure/path/to/wallet.json

# Password files
chmod 400 /secure/path/to/wallet.password
chown neo:neo /secure/path/to/wallet.password

# Log files
chmod 640 /var/log/neo-cpp/neo.log
chown neo:adm /var/log/neo-cpp/neo.log
```

## RPC Server Security

### Authentication
The RPC server supports HTTP Basic Authentication:

```json
{
  "rpc": {
    "enabled": true,
    "bind_address": "127.0.0.1",
    "port": 10332,
    "enable_authentication": true,
    "username": "admin",
    "password": "secure_password_from_environment"
  }
}
```

### Rate Limiting
Built-in rate limiting protects against abuse:

```json
{
  "rpc": {
    "enable_rate_limiting": true,
    "max_requests_per_second": 100,
    "max_request_size": 10485760
  }
}
```

### Method Restrictions
Disable dangerous methods in production:

```json
{
  "rpc": {
    "disabled_methods": [
      "submitblock",
      "sendrawtransaction",
      "invoke",
      "invokefunction",
      "invokescript"
    ]
  }
}
```

### TLS/SSL
Enable TLS for secure communication:

```json
{
  "rpc": {
    "ssl_cert_file": "/etc/ssl/certs/neo-cpp.crt",
    "ssl_key_file": "/etc/ssl/private/neo-cpp.key"
  }
}
```

## Network Security

### Firewall Configuration
Configure firewall rules to limit access:

```bash
# Allow P2P connections (adjust port as needed)
ufw allow 10333/tcp

# Allow RPC only from specific IPs (replace with actual IPs)
ufw allow from 192.168.1.0/24 to any port 10332

# Allow monitoring from specific monitoring systems
ufw allow from 10.0.0.100 to any port 9090
ufw allow from 10.0.0.100 to any port 8080

# Deny all other traffic
ufw default deny incoming
ufw enable
```

### Network Binding
Bind services to appropriate interfaces:

- **P2P Network**: Bind to `0.0.0.0` to accept connections from peers
- **RPC Server**: Bind to `127.0.0.1` for local access only, or specific interface for remote access
- **Monitoring**: Bind to `127.0.0.1` or management network interface

## Wallet Security

### Password Requirements
- Minimum 12 characters
- Mix of uppercase, lowercase, numbers, and symbols
- No dictionary words
- Unique per wallet

### Key Storage
- Store private keys in encrypted wallet files
- Use hardware security modules (HSMs) for consensus nodes
- Never store private keys in plain text
- Use secure key derivation functions

### Examples Updated
All example code has been updated to remove hardcoded passwords:

```cpp
// SECURE - Prompt for password
std::string wallet_password;
std::cout << "Enter wallet password: ";
std::getline(std::cin, wallet_password);

if (wallet_password.length() < 8)
{
    std::cerr << "Password must be at least 8 characters" << std::endl;
    return 1;
}

auto wallet = wallet::Wallet::Create("wallet.json", wallet_password, "My Wallet");
```

```cpp
// SECURE - Use environment variable
const char* wallet_password_env = std::getenv("WALLET_PASSWORD");
if (!wallet_password_env)
{
    std::cerr << "WALLET_PASSWORD environment variable not set" << std::endl;
    return 1;
}

auto wallet = wallet::Wallet::Create("wallet.json", wallet_password_env, "My Wallet");
```

## Logging Security

### Log Level
Use appropriate log levels in production:
- `info` for normal operations
- `warning` for concerning events
- `error` for error conditions
- Avoid `debug` and `trace` in production

### Sensitive Data
Never log sensitive information:
- Private keys
- Wallet passwords
- Authentication tokens
- User personal data

### Log Rotation
Configure proper log rotation:

```json
{
  "logging": {
    "file_output": true,
    "log_file_path": "/var/log/neo-cpp/neo.log",
    "max_file_size_mb": 100,
    "max_files": 10,
    "enable_file_rotation": true
  }
}
```

## Monitoring Security

### Health Endpoints
Health check endpoints are available at:

- `/health` - Detailed health information (JSON)
- `/ready` - Kubernetes readiness probe
- `/live` - Kubernetes liveness probe
- `/metrics` - Prometheus metrics

### Access Control
Restrict access to monitoring endpoints:

```bash
# Only allow monitoring systems
iptables -A INPUT -p tcp --dport 8080 -s 10.0.0.100 -j ACCEPT
iptables -A INPUT -p tcp --dport 8080 -j DROP

iptables -A INPUT -p tcp --dport 9090 -s 10.0.0.100 -j ACCEPT
iptables -A INPUT -p tcp --dport 9090 -j DROP
```

## System Security

### User Isolation
Run the node as a dedicated user:

```bash
# Create neo user
useradd -r -s /bin/false neo

# Set up directories
mkdir -p /var/lib/neo-cpp/data
mkdir -p /var/log/neo-cpp
mkdir -p /etc/neo-cpp

chown -R neo:neo /var/lib/neo-cpp
chown -R neo:adm /var/log/neo-cpp
chown -R neo:neo /etc/neo-cpp
```

### Resource Limits
Configure resource limits:

```bash
# /etc/security/limits.conf
neo soft nofile 65536
neo hard nofile 65536
neo soft nproc 32768
neo hard nproc 32768
```

### System Updates
Keep the system updated:

```bash
# Regular security updates
apt update && apt upgrade -y

# Monitor security advisories
apt install unattended-upgrades
```

## Container Security

### Docker Security
If running in containers:

```dockerfile
# Use non-root user
RUN groupadd -r neo && useradd -r -g neo neo
USER neo

# Drop capabilities
RUN setcap 'cap_net_bind_service=+ep' /usr/local/bin/neo-node

# Use read-only filesystem where possible
VOLUME ["/data"]
VOLUME ["/logs"]
```

### Kubernetes Security
Security context for Kubernetes:

```yaml
apiVersion: v1
kind: Pod
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 1000
  containers:
  - name: neo-cpp
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE
```

## Security Checklist

- [ ] Configuration loaded from environment variables
- [ ] No hardcoded passwords or keys
- [ ] RPC authentication enabled
- [ ] Rate limiting configured
- [ ] Dangerous RPC methods disabled
- [ ] TLS/SSL certificates configured
- [ ] Firewall rules applied
- [ ] Services bound to appropriate interfaces
- [ ] Dedicated user account created
- [ ] File permissions set correctly
- [ ] Log rotation configured
- [ ] Monitoring access restricted
- [ ] Resource limits applied
- [ ] Security updates automated
- [ ] Backup encryption enabled
- [ ] Incident response plan prepared

## Security Contacts

For security issues, contact:
- Security team: security@neo.org
- Bug bounty program: https://neo.org/security

## Regular Security Tasks

### Daily
- Monitor security logs
- Check resource usage
- Verify service health

### Weekly
- Review access logs
- Check for security updates
- Validate backup integrity

### Monthly
- Security configuration review
- Access control audit
- Penetration testing
- Update security documentation

### Quarterly
- Full security audit
- Disaster recovery testing
- Security training updates
- Threat model review