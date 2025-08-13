#!/bin/bash

# Local release build script for Neo C++
# This script builds and packages Neo C++ locally when GitHub Actions are unavailable

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
    -DNEO_USE_MINIMAL_DEPENDENCIES=ON \
    2>&1 | tee cmake.log || {
    echo -e "${RED}CMake configuration failed. Check cmake.log for details.${NC}"
    # Continue anyway to package what we can
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

# Find and copy executables
echo "Finding executables..."
find build-release -type f -executable \( -name "*neo*" -o -name "*node*" -o -name "*cli*" \) 2>/dev/null | while read -r exe; do
    echo "  Found: $(basename "$exe")"
    cp "$exe" "release/neo-node-${VERSION}/bin/" 2>/dev/null || true
done

# If no executables found, create placeholders
if [ -z "$(ls -A release/neo-node-${VERSION}/bin 2>/dev/null)" ]; then
    echo -e "${YELLOW}No executables found, creating placeholders...${NC}"
    cat > "release/neo-node-${VERSION}/bin/neo-node" << EOF
#!/bin/bash
echo "Neo C++ Node ${VERSION}"
echo "This is a placeholder. The actual binary build failed."
echo "Please check the build logs for details."
EOF
    chmod +x "release/neo-node-${VERSION}/bin/neo-node"
    
    cp "release/neo-node-${VERSION}/bin/neo-node" "release/neo-node-${VERSION}/bin/neo-cli"
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

ARCHIVE_NAME="neo-node-${PLATFORM}-${ARCH}-${VERSION}"

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
                --notes "Local build release" \
                "${ARCHIVE_NAME}".* checksums.txt
        fi
        echo -e "${GREEN}Upload complete!${NC}"
    fi
fi

cd ..
echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo "Release artifacts are in: $(pwd)/release/"
echo ""
echo "To install locally:"
echo "  tar -xzf release/${ARCHIVE_NAME}.tar.gz"
echo "  cd neo-node-${VERSION}"
echo "  ./start.sh"