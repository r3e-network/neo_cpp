#!/bin/bash

# Emergency release script for Neo C++
# Use this when GitHub Actions is down or slow

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Get version
VERSION=${1:-$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")}
if [[ ! "$VERSION" =~ ^v ]]; then
    VERSION="v${VERSION}"
fi

echo -e "${GREEN}Emergency Release Builder - Neo C++ ${VERSION}${NC}"
echo -e "${YELLOW}This script creates a release when GitHub Actions is unavailable${NC}"
echo ""

# Quick dependency check
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is required${NC}"
    exit 1
fi

# Clean previous builds
echo -e "${YELLOW}Cleaning previous builds...${NC}"
rm -rf build-emergency release-emergency

# Build
echo -e "${YELLOW}Building Neo C++ (this may take a few minutes)...${NC}"
mkdir -p build-emergency
cd build-emergency

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DNEO_BUILD_TESTS=OFF \
    -DNEO_BUILD_EXAMPLES=OFF \
    2>&1 | grep -E "^--|Building|Error" || true

cmake --build . --parallel 2>&1 | grep -E "^\[|Building|Error|Warning" || true

cd ..

# Check if build succeeded
if [ ! -f build-emergency/apps/neo_node ] && [ ! -f build-emergency/tools/neo_cli_tool ]; then
    echo -e "${RED}Build failed! Check the output above for errors.${NC}"
    exit 1
fi

# Package
echo -e "${YELLOW}Packaging binaries...${NC}"
mkdir -p release-emergency/neo-cpp-${VERSION}/{bin,config}

# Copy binaries
[ -f build-emergency/apps/neo_node ] && cp build-emergency/apps/neo_node release-emergency/neo-cpp-${VERSION}/bin/neo-node
[ -f build-emergency/tools/neo_cli_tool ] && cp build-emergency/tools/neo_cli_tool release-emergency/neo-cpp-${VERSION}/bin/neo-cli
[ -f build-emergency/tools/test_rpc_server ] && cp build-emergency/tools/test_rpc_server release-emergency/neo-cpp-${VERSION}/bin/neo-rpc

# Copy configs
cp config/*.json release-emergency/neo-cpp-${VERSION}/config/ 2>/dev/null || true

# Create quick start script
cat > release-emergency/neo-cpp-${VERSION}/start.sh << 'EOF'
#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$DIR/bin/neo-node" --config "$DIR/config/testnet.json" "$@"
EOF
chmod +x release-emergency/neo-cpp-${VERSION}/start.sh

# Detect platform
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m | sed 's/x86_64/x64/;s/aarch64/arm64/;s/arm64/arm64/')

# Create archive
cd release-emergency
ARCHIVE="neo-cpp-${PLATFORM}-${ARCH}-${VERSION}.tar.gz"
tar -czf "$ARCHIVE" neo-cpp-${VERSION}

# Generate checksum
shasum -a 256 "$ARCHIVE" > "${ARCHIVE}.sha256" 2>/dev/null || \
    shasum -a 256 "$ARCHIVE" > "${ARCHIVE}.sha256" 2>/dev/null || \
    echo "No checksum tool available"

cd ..

echo ""
echo -e "${GREEN}=== Emergency Build Complete ===${NC}"
echo "Archive: release-emergency/${ARCHIVE}"
echo "Size: $(du -h release-emergency/${ARCHIVE} | cut -f1)"
echo ""

# Offer to upload
if command -v gh &> /dev/null; then
    echo -e "${YELLOW}Upload to GitHub?${NC}"
    read -p "Create release ${VERSION} and upload? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        cd release-emergency
        
        # Create release notes
        cat > RELEASE.md << EOF
# Neo C++ ${VERSION} - Emergency Release

This is an emergency release created locally due to CI/CD issues.

## Platform
- OS: ${PLATFORM}
- Architecture: ${ARCH}
- Build date: $(date)

## Installation
\`\`\`bash
tar -xzf ${ARCHIVE}
cd neo-cpp-${VERSION}
./start.sh
\`\`\`
EOF
        
        # Create or update release
        if gh release view "${VERSION}" &>/dev/null; then
            echo "Updating existing release..."
            gh release upload "${VERSION}" "${ARCHIVE}" "${ARCHIVE}.sha256" --clobber
        else
            echo "Creating new release..."
            gh release create "${VERSION}" \
                --title "Neo C++ ${VERSION}" \
                --notes-file RELEASE.md \
                "${ARCHIVE}" "${ARCHIVE}.sha256"
        fi
        
        echo -e "${GREEN}Release uploaded successfully!${NC}"
        echo "View at: https://github.com/$(gh repo view --json nameWithOwner -q .nameWithOwner)/releases/tag/${VERSION}"
    fi
else
    echo -e "${YELLOW}GitHub CLI not found. Manual upload required:${NC}"
    echo "1. Go to: https://github.com/YOUR_REPO/releases/new"
    echo "2. Tag: ${VERSION}"
    echo "3. Upload: release-emergency/${ARCHIVE}"
fi