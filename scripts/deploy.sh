#!/bin/bash
# Production deployment script for Neo C++

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
INSTALL_PREFIX="/opt/neo-cpp"
SERVICE_USER="neo"
CONFIG_FILE="config/production_config.json"
CREATE_SERVICE="false"
START_SERVICE="false"

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -p, --prefix PATH     Installation prefix [default: /opt/neo-cpp]"
    echo "  -u, --user USER       Service user [default: neo]"
    echo "  -c, --config FILE     Configuration file [default: config/production_config.json]"
    echo "  -s, --service         Create systemd service"
    echo "  --start               Start service after installation"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -s --start         # Install with service and start"
    echo "  $0 -p /usr/local/neo  # Install to custom location"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -u|--user)
            SERVICE_USER="$2"
            shift 2
            ;;
        -c|--config)
            CONFIG_FILE="$2"
            shift 2
            ;;
        -s|--service)
            CREATE_SERVICE="true"
            shift
            ;;
        --start)
            START_SERVICE="true"
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    print_error "This script must be run as root for system installation"
    exit 1
fi

print_info "Starting Neo C++ deployment process"
print_info "Installation prefix: $INSTALL_PREFIX"
print_info "Service user: $SERVICE_USER"
print_info "Configuration file: $CONFIG_FILE"

# Check if build exists
if [[ ! -d "build" ]]; then
    print_error "Build directory not found. Please run build.sh first."
    exit 1
fi

if [[ ! -f "build/apps/cli/neo-cli" ]]; then
    print_error "neo-cli executable not found. Please build the project first."
    exit 1
fi

# Create service user
print_step "Creating service user"
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd --system --home-dir "$INSTALL_PREFIX" --shell /bin/false "$SERVICE_USER"
    print_info "Created user: $SERVICE_USER"
else
    print_info "User $SERVICE_USER already exists"
fi

# Create directories
print_step "Creating installation directories"
mkdir -p "$INSTALL_PREFIX"/{bin,config,data,logs}
mkdir -p /var/lib/neo-cpp
mkdir -p /var/log/neo-cpp

# Install binaries
print_step "Installing binaries"
cp build/apps/cli/neo-cli "$INSTALL_PREFIX/bin/"
cp build/apps/node/neo-node "$INSTALL_PREFIX/bin/" 2>/dev/null || true
chmod +x "$INSTALL_PREFIX/bin/"*

# Install configuration
print_step "Installing configuration"
if [[ -f "$CONFIG_FILE" ]]; then
    cp "$CONFIG_FILE" "$INSTALL_PREFIX/config/config.json"
else
    print_warning "Configuration file $CONFIG_FILE not found, creating default"
    cat > "$INSTALL_PREFIX/config/config.json" << 'EOF'
{
  "ApplicationConfiguration": {
    "Logger": {
      "Path": "/var/log/neo-cpp",
      "ConsoleOutput": false,
      "Active": true
    },
    "Storage": {
      "Engine": "LevelDBStore",
      "Path": "/var/lib/neo-cpp/data"
    },
    "P2P": {
      "Port": 10333,
      "MinDesiredConnections": 10,
      "MaxConnections": 40
    }
  },
  "ProtocolConfiguration": {
    "Network": 860833102,
    "AddressVersion": 53,
    "MillisecondsPerBlock": 15000,
    "MaxTransactionsPerBlock": 512
  }
}
EOF
fi

# Set permissions
print_step "Setting permissions"
chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_PREFIX"
chown -R "$SERVICE_USER:$SERVICE_USER" /var/lib/neo-cpp
chown -R "$SERVICE_USER:$SERVICE_USER" /var/log/neo-cpp
chmod 755 "$INSTALL_PREFIX/bin/"*

# Create systemd service
if [[ "$CREATE_SERVICE" == "true" ]]; then
    print_step "Creating systemd service"
    cat > /etc/systemd/system/neo-cpp.service << EOF
[Unit]
Description=Neo C++ Blockchain Node
After=network.target
Wants=network-online.target

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=$INSTALL_PREFIX
ExecStart=$INSTALL_PREFIX/bin/neo-cli --config $INSTALL_PREFIX/config/config.json --daemon
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=neo-cpp

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/neo-cpp /var/log/neo-cpp
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    systemctl enable neo-cpp
    print_info "Systemd service created and enabled"
fi

# Create logrotate configuration
print_step "Setting up log rotation"
cat > /etc/logrotate.d/neo-cpp << 'EOF'
/var/log/neo-cpp/*.log {
    daily
    missingok
    rotate 52
    compress
    delaycompress
    notifempty
    create 644 neo neo
    postrotate
        systemctl reload neo-cpp 2>/dev/null || true
    endscript
}
EOF

# Create firewall rules (if ufw is available)
if command -v ufw >/dev/null 2>&1; then
    print_step "Configuring firewall"
    ufw allow 10333/tcp comment "Neo P2P"
    ufw allow 20333/tcp comment "Neo RPC"
    print_info "Firewall rules added"
fi

# Start service if requested
if [[ "$START_SERVICE" == "true" && "$CREATE_SERVICE" == "true" ]]; then
    print_step "Starting Neo C++ service"
    systemctl start neo-cpp
    sleep 2
    if systemctl is-active --quiet neo-cpp; then
        print_info "Neo C++ service started successfully"
    else
        print_error "Failed to start Neo C++ service"
        systemctl status neo-cpp
        exit 1
    fi
fi

print_info "Neo C++ deployment completed successfully!"
print_info ""
print_info "Installation details:"
print_info "  Binaries: $INSTALL_PREFIX/bin/"
print_info "  Configuration: $INSTALL_PREFIX/config/"
print_info "  Data directory: /var/lib/neo-cpp/"
print_info "  Log directory: /var/log/neo-cpp/"

if [[ "$CREATE_SERVICE" == "true" ]]; then
    print_info ""
    print_info "Service management:"
    print_info "  Start service:   systemctl start neo-cpp"
    print_info "  Stop service:    systemctl stop neo-cpp"
    print_info "  Restart service: systemctl restart neo-cpp"
    print_info "  Check status:    systemctl status neo-cpp"
    print_info "  View logs:       journalctl -u neo-cpp -f"
fi