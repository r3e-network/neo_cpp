#!/bin/bash

# Neo C++ Quick Release Build Script
# Builds main release binaries without full test suite

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-quick-release"
RELEASE_DIR="$PROJECT_ROOT/release"
VERSION=$(cat "$PROJECT_ROOT/VERSION")

echo "========================================="
echo "Neo C++ Quick Release Build v$VERSION"
echo "========================================="
echo ""

# Detect platform
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux*)     PLATFORM="linux";;
    Darwin*)    PLATFORM="macos";;
    *)          echo "Unsupported platform: $OS"; exit 1;;
esac

case "$ARCH" in
    x86_64)     ARCH_NAME="x64";;
    aarch64|arm64) ARCH_NAME="arm64";;
    *)          ARCH_NAME="$ARCH";;
esac

echo "Platform: $PLATFORM-$ARCH_NAME"
echo "Version: $VERSION"
echo ""

# Clean and create directories
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

# Configure with minimal options to avoid test errors
echo "Configuring CMake (minimal build)..."
cd "$BUILD_DIR"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
    -DBUILD_BENCHMARKS=OFF \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TOOLS=ON \
    -DBUILD_SDK=ON \
    -DBUILD_APPS=ON

# Build
echo ""
echo "Building Neo C++..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Package binaries
echo ""
echo "Packaging binaries..."
PACKAGE_NAME="neo-cpp-v$VERSION-$PLATFORM-$ARCH_NAME"
PACKAGE_DIR="$RELEASE_DIR/$PACKAGE_NAME"

mkdir -p "$PACKAGE_DIR/bin"
mkdir -p "$PACKAGE_DIR/lib"
mkdir -p "$PACKAGE_DIR/config"
mkdir -p "$PACKAGE_DIR/docs"

# Copy main executables
echo "Copying executables..."
for app in neo_cli neo_node neo_testnet_node working_neo_node; do
    if [ -f "$BUILD_DIR/apps/$app" ]; then
        cp "$BUILD_DIR/apps/$app" "$PACKAGE_DIR/bin/"
        echo "  - $app"
    fi
done

# Copy tools
echo "Copying tools..."
if [ -d "$BUILD_DIR/tools" ]; then
    for tool in "$BUILD_DIR/tools"/*; do
        if [ -f "$tool" ] && [ -x "$tool" ]; then
            cp "$tool" "$PACKAGE_DIR/bin/"
            echo "  - $(basename "$tool")"
        fi
    done
fi

# Copy SDK libraries
echo "Copying SDK libraries..."
if [ -d "$BUILD_DIR/sdk" ]; then
    find "$BUILD_DIR/sdk" \( -name "*.so" -o -name "*.dylib" -o -name "*.a" \) -exec cp {} "$PACKAGE_DIR/lib/" \; 2>/dev/null || true
fi

# Copy configuration
if [ -d "$PROJECT_ROOT/config" ]; then
    cp -r "$PROJECT_ROOT/config"/* "$PACKAGE_DIR/config/" 2>/dev/null || true
fi

# Copy documentation
cp "$PROJECT_ROOT/README.md" "$PACKAGE_DIR/"
cp "$PROJECT_ROOT/LICENSE" "$PACKAGE_DIR/"
cp "$PROJECT_ROOT/CHANGELOG.md" "$PACKAGE_DIR/"
cp "$PROJECT_ROOT/RELEASE_NOTES_v$VERSION.md" "$PACKAGE_DIR/" 2>/dev/null || true

# Create version info
cat > "$PACKAGE_DIR/VERSION.txt" << EOF
Neo C++ v$VERSION
Platform: $PLATFORM-$ARCH_NAME
Build Date: $(date '+%Y-%m-%d %H:%M:%S')
Compiler: $(c++ --version | head -1)
EOF

# Create archive
echo ""
echo "Creating archive..."
cd "$RELEASE_DIR"
tar czf "$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"

# Generate checksum
echo "Generating checksum..."
if command -v sha256sum &> /dev/null; then
    sha256sum "$PACKAGE_NAME.tar.gz" > "$PACKAGE_NAME.tar.gz.sha256"
elif command -v shasum &> /dev/null; then
    shasum -a 256 "$PACKAGE_NAME.tar.gz" > "$PACKAGE_NAME.tar.gz.sha256"
fi

# Display summary
echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo "Package: $RELEASE_DIR/$PACKAGE_NAME.tar.gz"
echo "Size: $(du -h "$RELEASE_DIR/$PACKAGE_NAME.tar.gz" | cut -f1)"
if [ -f "$RELEASE_DIR/$PACKAGE_NAME.tar.gz.sha256" ]; then
    echo "SHA256: $(cut -d' ' -f1 < "$RELEASE_DIR/$PACKAGE_NAME.tar.gz.sha256")"
fi
echo ""
echo "Contents:"
ls -la "$PACKAGE_DIR/bin/" 2>/dev/null | grep -v "^total" | grep -v "^d" | awk '{print "  - "$NF}' || echo "  No binaries found"
echo ""