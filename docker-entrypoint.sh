#!/bin/bash
# Docker entrypoint script for Neo C++ node

set -e

# Network configuration
NETWORK=${1:-mainnet}
CONFIG_FILE="/etc/neo/config.json"

# Select configuration based on network
case "$NETWORK" in
    mainnet)
        echo "Starting Neo node on MainNet..."
        CONFIG_FILE="/etc/neo/mainnet.json"
        ;;
    testnet)
        echo "Starting Neo node on TestNet..."
        CONFIG_FILE="/etc/neo/testnet.json"
        ;;
    private)
        echo "Starting Neo node on Private network..."
        CONFIG_FILE="/etc/neo/private.json"
        ;;
    *)
        echo "Unknown network: $NETWORK. Using default configuration."
        ;;
esac

# Check if config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Configuration file not found: $CONFIG_FILE"
    echo "Using default configuration..."
    CONFIG_FILE="/etc/neo/config.json"
fi

# Set environment variables
export NEO_CONFIG_FILE="$CONFIG_FILE"
export NEO_DATA_PATH="${NEO_DATA_PATH:-/var/lib/neo}"
export NEO_LOG_PATH="${NEO_LOG_PATH:-/var/log/neo}"

# Create directories if they don't exist
mkdir -p "$NEO_DATA_PATH" "$NEO_LOG_PATH"

# Execute neo-node or neo_cli_tool based on available binary
if [ -f "/usr/local/bin/neo-node" ]; then
    exec neo-node --config "$CONFIG_FILE" "${@:2}"
elif [ -f "/opt/neo/neo_node" ]; then
    exec /opt/neo/neo_node --config "$CONFIG_FILE" "${@:2}"
elif [ -f "/usr/local/bin/neo_cli_tool" ]; then
    exec neo_cli_tool start --config "$CONFIG_FILE" "${@:2}"
elif [ -f "/opt/neo/neo_cli_tool" ]; then
    exec /opt/neo/neo_cli_tool start --config "$CONFIG_FILE" "${@:2}"
else
    echo "Error: No Neo executable found!"
    exit 1
fi