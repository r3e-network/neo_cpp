#!/bin/bash
# Neo C++ Installation Script
# Supports multiple installation methods and platforms

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
INSTALL_PREFIX="${INSTALL_PREFIX:-/opt/neo-cpp}"
NEO_USER="${NEO_USER:-neo}"
NEO_GROUP="${NEO_GROUP:-neo}"
NEO_VERSION="${NEO_VERSION:-latest}"
INSTALL_METHOD="${INSTALL_METHOD:-binary}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Functions
log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_step() { echo -e "${BLUE}[STEP]${NC} $1"; }

# Detect OS
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$ID
        VER=$VERSION_ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
        VER=$(lsb_release -sr)
    else
        log_error "Cannot detect OS"
        exit 1
    fi
    
    log_info "Detected OS: $OS $VER"
}

# Install dependencies
install_dependencies() {
    log_step "Installing dependencies"
    
    case $OS in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                curl \
                wget \
                jq \
                libboost-all-dev \
                libssl-dev \
                librocksdb-dev \
                libsnappy-dev \
                liblz4-dev \
                libzstd-dev
            ;;
        centos|rhel|fedora)
            sudo yum install -y epel-release
            sudo yum install -y \
                curl \
                wget \
                jq \
                boost-devel \
                openssl-devel \
                rocksdb-devel \
                snappy-devel \
                lz4-devel \
                libzstd-devel
            ;;
        arch|manjaro)
            sudo pacman -Sy --noconfirm \
                curl \
                wget \
                jq \
                boost \
                openssl \
                rocksdb \
                snappy \
                lz4 \
                zstd
            ;;
        darwin)
            if ! command -v brew &> /dev/null; then
                log_error "Homebrew not installed"
                exit 1
            fi
            brew install \
                boost \
                openssl \
                rocksdb \
                snappy \
                lz4 \
                zstd
            ;;
        *)
            log_error "Unsupported OS: $OS"
            exit 1
            ;;
    esac
}

# Create user and directories
create_user_and_dirs() {
    log_step "Creating user and directories"
    
    # Create user (skip on macOS)
    if [[ "$OS" != "darwin" ]]; then
        if ! id "$NEO_USER" &>/dev/null; then
            sudo useradd -r -s /bin/false -m -d "$INSTALL_PREFIX" "$NEO_USER"
            log_info "Created user: $NEO_USER"
        fi
    fi
    
    # Create directories
    sudo mkdir -p "$INSTALL_PREFIX"/{bin,lib,config,data,logs}
    sudo mkdir -p /var/lib/neo /var/log/neo /etc/neo
    
    # Set permissions
    if [[ "$OS" != "darwin" ]]; then
        sudo chown -R "$NEO_USER:$NEO_GROUP" "$INSTALL_PREFIX" /var/lib/neo /var/log/neo /etc/neo
    fi
}

# Download binary release
install_binary() {
    log_step "Installing from binary release"
    
    # Determine architecture
    ARCH=$(uname -m)
    case $ARCH in
        x86_64) ARCH="amd64" ;;
        aarch64) ARCH="arm64" ;;
        *) log_error "Unsupported architecture: $ARCH"; exit 1 ;;
    esac
    
    # Determine OS suffix
    case $OS in
        ubuntu|debian|centos|rhel|fedora|arch) OS_SUFFIX="linux" ;;
        darwin) OS_SUFFIX="darwin" ;;
        *) log_error "Unsupported OS for binary: $OS"; exit 1 ;;
    esac
    
    # Download release
    if [[ "$NEO_VERSION" == "latest" ]]; then
        DOWNLOAD_URL=$(curl -s https://api.github.com/repos/neo-project/neo-cpp/releases/latest | \
            jq -r ".assets[] | select(.name | contains(\"$OS_SUFFIX-$ARCH\")) | .browser_download_url")
    else
        DOWNLOAD_URL="https://github.com/neo-project/neo-cpp/releases/download/${NEO_VERSION}/neo-cpp-${NEO_VERSION}-${OS_SUFFIX}-${ARCH}.tar.gz"
    fi
    
    log_info "Downloading from: $DOWNLOAD_URL"
    wget -O /tmp/neo-cpp.tar.gz "$DOWNLOAD_URL"
    
    # Extract
    sudo tar -xzf /tmp/neo-cpp.tar.gz -C "$INSTALL_PREFIX"
    rm /tmp/neo-cpp.tar.gz
    
    # Make binaries executable
    sudo chmod +x "$INSTALL_PREFIX"/bin/*
}

# Build from source
install_source() {
    log_step "Building from source"
    
    # Clone repository if not in project directory
    if [[ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]]; then
        git clone https://github.com/neo-project/neo-cpp.git /tmp/neo-cpp
        PROJECT_ROOT="/tmp/neo-cpp"
    fi
    
    cd "$PROJECT_ROOT"
    
    # Build
    mkdir -p build
    cd build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DNEO_BUILD_TESTS=OFF
    make -j$(nproc)
    sudo make install
}

# Install Docker
install_docker() {
    log_step "Installing Docker version"
    
    if ! command -v docker &> /dev/null; then
        log_error "Docker not installed"
        exit 1
    fi
    
    # Pull image
    docker pull neo-cpp:latest
    
    # Create docker-compose file
    cat > /tmp/docker-compose.yml <<EOF
version: '3.8'
services:
  neo-node:
    image: neo-cpp:latest
    container_name: neo-node
    restart: unless-stopped
    ports:
      - "10332:10332"
      - "10334:10334"
    volumes:
      - neo-data:/var/lib/neo
      - ./config:/etc/neo:ro
    environment:
      - NEO_NETWORK=mainnet
volumes:
  neo-data:
EOF
    
    sudo mv /tmp/docker-compose.yml /etc/neo/
    log_info "Docker compose file created at /etc/neo/docker-compose.yml"
}

# Configure systemd service
configure_systemd() {
    log_step "Configuring systemd service"
    
    if [[ "$OS" == "darwin" ]]; then
        log_warn "Systemd not available on macOS, skipping"
        return
    fi
    
    # Copy service file
    if [[ -f "$PROJECT_ROOT/deploy/systemd/neo-node.service" ]]; then
        sudo cp "$PROJECT_ROOT/deploy/systemd/neo-node.service" /etc/systemd/system/
    else
        # Create minimal service file
        cat | sudo tee /etc/systemd/system/neo-node.service > /dev/null <<EOF
[Unit]
Description=Neo C++ Node
After=network.target

[Service]
Type=simple
User=$NEO_USER
Group=$NEO_GROUP
ExecStart=$INSTALL_PREFIX/bin/neo-node --config /etc/neo/config.json
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
    fi
    
    sudo systemctl daemon-reload
    sudo systemctl enable neo-node
    log_info "Systemd service configured"
}

# Configure launchd (macOS)
configure_launchd() {
    log_step "Configuring launchd service"
    
    cat > ~/Library/LaunchAgents/org.neo.node.plist <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>org.neo.node</string>
    <key>ProgramArguments</key>
    <array>
        <string>$INSTALL_PREFIX/bin/neo-node</string>
        <string>--config</string>
        <string>/etc/neo/config.json</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/var/log/neo/node.log</string>
    <key>StandardErrorPath</key>
    <string>/var/log/neo/node.error.log</string>
</dict>
</plist>
EOF
    
    launchctl load ~/Library/LaunchAgents/org.neo.node.plist
    log_info "Launchd service configured"
}

# Create default configuration
create_config() {
    log_step "Creating default configuration"
    
    cat | sudo tee /etc/neo/config.json > /dev/null <<EOF
{
  "ApplicationConfiguration": {
    "Network": "mainnet",
    "Storage": {
      "Engine": "rocksdb",
      "Path": "/var/lib/neo/chain"
    },
    "P2P": {
      "Port": 10332,
      "MaxConnections": 100
    },
    "RPC": {
      "Enabled": true,
      "Port": 10334
    },
    "Logging": {
      "Level": "info",
      "Path": "/var/log/neo"
    }
  }
}
EOF
    
    log_info "Configuration created at /etc/neo/config.json"
}

# Print usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -m, --method METHOD     Installation method (binary|source|docker) [default: binary]"
    echo "  -v, --version VERSION   Neo version to install [default: latest]"
    echo "  -p, --prefix PATH       Installation prefix [default: /opt/neo-cpp]"
    echo "  -u, --user USER         Service user [default: neo]"
    echo "  -s, --start             Start service after installation"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                      # Install latest binary release"
    echo "  $0 -m source            # Build from source"
    echo "  $0 -v v1.0.0 -s         # Install specific version and start"
}

# Main
main() {
    local start_service=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -m|--method)
                INSTALL_METHOD="$2"
                shift 2
                ;;
            -v|--version)
                NEO_VERSION="$2"
                shift 2
                ;;
            -p|--prefix)
                INSTALL_PREFIX="$2"
                shift 2
                ;;
            -u|--user)
                NEO_USER="$2"
                shift 2
                ;;
            -s|--start)
                start_service=true
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
    
    # Start installation
    log_info "Starting Neo C++ installation"
    log_info "Method: $INSTALL_METHOD"
    log_info "Version: $NEO_VERSION"
    log_info "Prefix: $INSTALL_PREFIX"
    
    # Detect OS
    detect_os
    
    # Install dependencies
    install_dependencies
    
    # Create user and directories
    create_user_and_dirs
    
    # Install based on method
    case $INSTALL_METHOD in
        binary)
            install_binary
            ;;
        source)
            install_source
            ;;
        docker)
            install_docker
            ;;
        *)
            log_error "Unknown installation method: $INSTALL_METHOD"
            exit 1
            ;;
    esac
    
    # Create configuration
    create_config
    
    # Configure service
    if [[ "$INSTALL_METHOD" != "docker" ]]; then
        if [[ "$OS" == "darwin" ]]; then
            configure_launchd
        else
            configure_systemd
        fi
    fi
    
    # Start service if requested
    if [[ "$start_service" == "true" ]]; then
        log_step "Starting Neo node"
        if [[ "$OS" == "darwin" ]]; then
            launchctl start org.neo.node
        elif [[ "$INSTALL_METHOD" == "docker" ]]; then
            cd /etc/neo && docker-compose up -d
        else
            sudo systemctl start neo-node
        fi
        log_info "Neo node started"
    fi
    
    # Print summary
    log_info "Installation completed successfully!"
    log_info ""
    log_info "Installation details:"
    log_info "  Binaries: $INSTALL_PREFIX/bin/"
    log_info "  Configuration: /etc/neo/config.json"
    log_info "  Data: /var/lib/neo/"
    log_info "  Logs: /var/log/neo/"
    log_info ""
    
    if [[ "$INSTALL_METHOD" != "docker" ]]; then
        if [[ "$OS" == "darwin" ]]; then
            log_info "Service management:"
            log_info "  Start: launchctl start org.neo.node"
            log_info "  Stop: launchctl stop org.neo.node"
            log_info "  Status: launchctl list | grep neo"
        else
            log_info "Service management:"
            log_info "  Start: sudo systemctl start neo-node"
            log_info "  Stop: sudo systemctl stop neo-node"
            log_info "  Status: sudo systemctl status neo-node"
            log_info "  Logs: sudo journalctl -u neo-node -f"
        fi
    else
        log_info "Docker management:"
        log_info "  Start: cd /etc/neo && docker-compose up -d"
        log_info "  Stop: cd /etc/neo && docker-compose down"
        log_info "  Logs: cd /etc/neo && docker-compose logs -f"
    fi
}

# Run main
main "$@"