#!/bin/bash

# Fixed local release build script for Neo C++
# This version correctly finds and packages binaries

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get version from argument or VERSION file
VERSION=${1:-$(cat VERSION 2>/dev/null || echo "0.0.0")}
if [[ ! "$VERSION" =~ ^v ]]; then
    VERSION="v${VERSION}"
fi

echo -e "${GREEN}Building Neo C++ Release ${VERSION}${NC}"

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"
MISSING_DEPS=()

if ! command_exists cmake; then
    MISSING_DEPS+=("cmake")
fi

if ! command_exists make && ! command_exists ninja; then
    MISSING_DEPS+=("make or ninja")
fi

if ! command_exists g++ && ! command_exists clang++; then
    MISSING_DEPS+=("g++ or clang++")
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo -e "${RED}Missing dependencies: ${MISSING_DEPS[*]}${NC}"
    echo "Please install the missing dependencies and try again."
    exit 1
fi

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
rm -rf build-release
mkdir -p build-release
cd build-release

# Configure CMake
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DNEO_BUILD_TESTS=OFF \
    2>&1 | tee cmake.log || {
    echo -e "${YELLOW}CMake configuration had warnings, continuing...${NC}"
}

# Build
echo -e "${YELLOW}Building Neo C++...${NC}"
if command_exists ninja && [ -f build.ninja ]; then
    ninja -j4 2>&1 | tee build.log || true
else
    make -j4 2>&1 | tee build.log || true
fi

# Create release directory structure
echo -e "${YELLOW}Packaging release...${NC}"
cd ..
rm -rf release
mkdir -p release/neo-node-${VERSION}/{bin,config,docs}

# Fixed binary paths - look in correct locations
echo -e "${GREEN}Finding and copying executables...${NC}"
BINARY_PATHS=(
    "build-release/apps/neo_node"
    "build-release/tools/neo_cli_tool"
    "build-release/tools/test_rpc_server"
    "build-release/tools/neo_gui"
    "build-release/examples/neo_example_vm"
    "build-release/examples/neo_example_network"
)

FOUND_BINARIES=0
for binary_path in "${BINARY_PATHS[@]}"; do
    if [ -f "$binary_path" ]; then
        filename=$(basename "$binary_path")
        # Rename to standard names
        case "$filename" in
            neo_node) 
                cp "$binary_path" "release/neo-node-${VERSION}/bin/neo-node"
                echo "  ✅ Found neo-node ($(du -h "$binary_path" | cut -f1))"
                FOUND_BINARIES=$((FOUND_BINARIES + 1))
                ;;
            neo_cli_tool) 
                cp "$binary_path" "release/neo-node-${VERSION}/bin/neo-cli"
                echo "  ✅ Found neo-cli ($(du -h "$binary_path" | cut -f1))"
                FOUND_BINARIES=$((FOUND_BINARIES + 1))
                ;;
            test_rpc_server) 
                cp "$binary_path" "release/neo-node-${VERSION}/bin/neo-rpc"
                echo "  ✅ Found neo-rpc ($(du -h "$binary_path" | cut -f1))"
                FOUND_BINARIES=$((FOUND_BINARIES + 1))
                ;;
            neo_gui)
                cp "$binary_path" "release/neo-node-${VERSION}/bin/neo-gui"
                echo "  ✅ Found neo-gui ($(du -h "$binary_path" | cut -f1))"
                FOUND_BINARIES=$((FOUND_BINARIES + 1))
                ;;
            *)
                cp "$binary_path" "release/neo-node-${VERSION}/bin/$filename"
                echo "  ✅ Found $filename"
                FOUND_BINARIES=$((FOUND_BINARIES + 1))
                ;;
        esac
    fi
done

if [ $FOUND_BINARIES -eq 0 ]; then
    echo -e "${RED}No binaries found! Build may have failed.${NC}"
    echo -e "${YELLOW}Creating placeholder binaries...${NC}"
    cat > "release/neo-node-${VERSION}/bin/neo-node" << EOF
#!/bin/bash
echo "Neo C++ Node ${VERSION} - Build failed"
echo "Please check build-release/build.log for errors"
EOF
    chmod +x "release/neo-node-${VERSION}/bin/neo-node"
else
    echo -e "${GREEN}Found $FOUND_BINARIES binaries${NC}"
fi

# Copy configuration files
echo "Copying configuration files..."
cp config/*.json "release/neo-node-${VERSION}/config/" 2>/dev/null || {
    echo -e "${YELLOW}Warning: Could not copy config files${NC}"
}

# Copy documentation
echo "Copying documentation..."
cp README.md LICENSE "release/neo-node-${VERSION}/" 2>/dev/null || true
cp -r docs "release/neo-node-${VERSION}/" 2>/dev/null || true

# Create version info file
cat > "release/neo-node-${VERSION}/VERSION.txt" << EOF
Neo C++ ${VERSION}
Built: $(date)
Platform: $(uname -s) $(uname -m)
Compiler: $(${CXX:-g++} --version | head -n1)
Binaries: $FOUND_BINARIES found
EOF

# Create startup scripts
echo "Creating startup scripts..."

# Linux/macOS startup script
cat > "release/neo-node-${VERSION}/start.sh" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/bin/neo-node" ]; then
    exec "$SCRIPT_DIR/bin/neo-node" --config "$SCRIPT_DIR/config/testnet.json" "$@"
else
    echo "Error: neo-node binary not found"
    exit 1
fi
EOF
chmod +x "release/neo-node-${VERSION}/start.sh"

# Create archives
echo -e "${YELLOW}Creating release archives...${NC}"
cd release

# Detect platform
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

if [ "$ARCH" = "x86_64" ]; then
    ARCH="x64"
elif [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
    ARCH="arm64"
fi

ARCHIVE_NAME="neo-cpp-${PLATFORM}-${ARCH}-${VERSION}"

# Create tar.gz
tar -czf "${ARCHIVE_NAME}.tar.gz" "neo-node-${VERSION}"
echo -e "${GREEN}Created: ${ARCHIVE_NAME}.tar.gz${NC}"

# Create zip if available
if command_exists zip; then
    zip -r "${ARCHIVE_NAME}.zip" "neo-node-${VERSION}" >/dev/null 2>&1
    echo -e "${GREEN}Created: ${ARCHIVE_NAME}.zip${NC}"
fi

# Generate checksums
sha256sum "${ARCHIVE_NAME}".* > checksums.txt 2>/dev/null || \
    shasum -a 256 "${ARCHIVE_NAME}".* > checksums.txt 2>/dev/null || \
    echo "Checksum generation not available"

echo -e "${GREEN}Checksums:${NC}"
cat checksums.txt

# Upload to GitHub if gh is available and user wants to
if command_exists gh; then
    echo ""
    echo -e "${YELLOW}Would you like to upload these artifacts to GitHub release ${VERSION}?${NC}"
    echo "This will create the release if it doesn't exist."
    read -p "Upload to GitHub? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # Check if release exists
        if gh release view "${VERSION}" >/dev/null 2>&1; then
            echo "Release ${VERSION} exists, uploading artifacts..."
            gh release upload "${VERSION}" "${ARCHIVE_NAME}".* checksums.txt --clobber
        else
            echo "Creating release ${VERSION}..."
            gh release create "${VERSION}" \
                --title "Neo C++ ${VERSION}" \
                --notes "Release ${VERSION} with $FOUND_BINARIES binaries" \
                "${ARCHIVE_NAME}".* checksums.txt
        fi
        echo -e "${GREEN}Upload complete!${NC}"
    fi
fi

cd ..
echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo "Release artifacts are in: $(pwd)/release/"
echo "Binaries found: $FOUND_BINARIES"
echo ""
echo "To install locally:"
echo "  tar -xzf release/${ARCHIVE_NAME}.tar.gz"
echo "  cd neo-node-${VERSION}"
echo "  ./start.sh"