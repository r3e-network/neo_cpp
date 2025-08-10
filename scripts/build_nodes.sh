#!/bin/bash

# Neo C++ Node Build Script
# This script builds all Neo node executables and provides quick access to them

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-build}
INSTALL_DIR=${INSTALL_DIR:-install}
NUM_JOBS=${NUM_JOBS:-$(nproc)}

echo -e "${BLUE}Neo C++ Node Build Script${NC}"
echo "=================================="

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the neo-cpp root directory."
    exit 1
fi

# Create build directory
print_status "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring with CMake (Build Type: $BUILD_TYPE)"
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="../$INSTALL_DIR" \
    -DNEO_BUILD_TESTS=OFF \
    -DNEO_BUILD_EXAMPLES=OFF \
    -DNEO_BUILD_TOOLS=ON \
    -DNEO_BUILD_APPS=ON \
    -DNEO_PRODUCTION_BUILD=ON

# Check if configuration was successful
if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"
    exit 1
fi

# Build the project
print_status "Building with $NUM_JOBS parallel jobs"
cmake --build . --parallel "$NUM_JOBS"

# Install to local directory
print_status "Installing to ../$INSTALL_DIR"
cmake --build . --target install

# Go back to root directory
cd ..

# Check if executables were built successfully
EXECUTABLES=(
    "$BUILD_DIR/apps/simple_neo_node"
    "$BUILD_DIR/apps/neo_node"
)

print_status "Checking built executables:"
for exe in "${EXECUTABLES[@]}"; do
    if [ -f "$exe" ]; then
        echo -e "  ${GREEN}✓${NC} $(basename "$exe")"
    else
        echo -e "  ${RED}✗${NC} $(basename "$exe") - not found"
    fi
done

# Create convenience scripts
print_status "Creating convenience scripts in ./bin/"
mkdir -p bin

# Simple node script
cat > bin/run_simple_node.sh << 'EOF'
#!/bin/bash
echo "Starting Simple Neo Node..."
echo "This is a lightweight implementation for development and testing."
echo "Press Ctrl+C to stop the node."
echo ""
exec ./build/apps/simple_neo_node "$@"
EOF

# Working node script
cat > bin/run_working_node.sh << 'EOF'
#!/bin/bash
echo "Starting Working Neo Node..."
echo "This is a full-featured implementation for testing environments."
echo "Press Ctrl+C to stop the node."
echo ""
exec ./build/apps/neo_node "$@"
EOF

# Production node script
cat > bin/run_production_node.sh << 'EOF'
#!/bin/bash
CONFIG_FILE=${1:-config/production_config.json}

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Configuration file '$CONFIG_FILE' not found."
    echo "Usage: $0 [config_file]"
    echo "Default: $0 config/production_config.json"
    exit 1
fi

echo "Starting Production Neo Node..."
echo "Configuration: $CONFIG_FILE"
echo "Press Ctrl+C to stop the node."
echo ""
exec ./build/apps/neo_node "$CONFIG_FILE"
EOF

# Make scripts executable
chmod +x bin/*.sh

print_status "Build completed successfully!"
echo ""
echo -e "${BLUE}Available executables:${NC}"
echo "  Simple Node:     ./bin/run_simple_node.sh"
echo "  Working Node:    ./bin/run_working_node.sh"  
echo "  Production Node: ./bin/run_production_node.sh [config_file]"
echo ""
echo -e "${BLUE}Direct executables:${NC}"
echo "  Simple Node:     ./build/apps/simple_neo_node"
echo "  Node:            ./build/apps/neo_node"
echo ""
echo -e "${YELLOW}Note:${NC} Production node requires a configuration file."
echo "Example configs are available in the config/ directory."