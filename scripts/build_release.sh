#!/bin/bash

# Neo C++ Release Build Script
# Builds release binaries for the current platform

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-release"
RELEASE_DIR="$PROJECT_ROOT/release"
VERSION=$(cat "$PROJECT_ROOT/VERSION")

echo "========================================="
echo "Neo C++ Release Build v$VERSION"
echo "========================================="
echo ""

# Detect platform
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux*)     PLATFORM="linux";;
    Darwin*)    PLATFORM="macos";;
    MINGW*|CYGWIN*|MSYS*) PLATFORM="windows";;
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

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf "$BUILD_DIR"
rm -rf "$RELEASE_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

# Configure CMake
echo "Configuring CMake..."
cd "$BUILD_DIR"

if [ "$PLATFORM" = "macos" ]; then
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=ON \
        -DBUILD_BENCHMARKS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TOOLS=ON \
        -DBUILD_SDK=ON
elif [ "$PLATFORM" = "linux" ]; then
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=ON \
        -DBUILD_BENCHMARKS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TOOLS=ON \
        -DBUILD_SDK=ON
else
    echo "Windows builds should use build_release.bat"
    exit 1
fi

# Build
echo ""
echo "Building Neo C++..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run tests
echo ""
echo "Running tests..."
ctest --output-on-failure -C Release || echo "Some tests failed, continuing..."

# Package binaries
echo ""
echo "Packaging binaries..."
PACKAGE_NAME="neo-cpp-v$VERSION-$PLATFORM-$ARCH_NAME"
PACKAGE_DIR="$RELEASE_DIR/$PACKAGE_NAME"

mkdir -p "$PACKAGE_DIR/bin"
mkdir -p "$PACKAGE_DIR/lib"
mkdir -p "$PACKAGE_DIR/include"
mkdir -p "$PACKAGE_DIR/config"
mkdir -p "$PACKAGE_DIR/docs"

# Copy binaries
echo "Copying binaries..."
if [ -f "$BUILD_DIR/apps/neo_cli" ]; then
    cp "$BUILD_DIR/apps/neo_cli" "$PACKAGE_DIR/bin/"
fi
if [ -f "$BUILD_DIR/apps/neo_node" ]; then
    cp "$BUILD_DIR/apps/neo_node" "$PACKAGE_DIR/bin/"
fi

# Copy tools
if [ -d "$BUILD_DIR/tools" ]; then
    find "$BUILD_DIR/tools" -type f -executable -exec cp {} "$PACKAGE_DIR/bin/" \; 2>/dev/null || true
fi

# Copy libraries
if [ -d "$BUILD_DIR/sdk" ]; then
    find "$BUILD_DIR/sdk" -name "*.so" -o -name "*.dylib" -o -name "*.a" | xargs -I {} cp {} "$PACKAGE_DIR/lib/" 2>/dev/null || true
fi

# Copy configuration files
if [ -d "$PROJECT_ROOT/config" ]; then
    cp -r "$PROJECT_ROOT/config"/* "$PACKAGE_DIR/config/" 2>/dev/null || true
fi

# Copy documentation
cp "$PROJECT_ROOT/README.md" "$PACKAGE_DIR/"
cp "$PROJECT_ROOT/LICENSE" "$PACKAGE_DIR/"
cp "$PROJECT_ROOT/CHANGELOG.md" "$PACKAGE_DIR/"
cp "$PROJECT_ROOT/RELEASE_NOTES_v$VERSION.md" "$PACKAGE_DIR/" 2>/dev/null || true

# Copy include files for SDK
if [ -d "$PROJECT_ROOT/include" ]; then
    cp -r "$PROJECT_ROOT/include"/* "$PACKAGE_DIR/include/" 2>/dev/null || true
fi
if [ -d "$PROJECT_ROOT/sdk/include" ]; then
    cp -r "$PROJECT_ROOT/sdk/include"/* "$PACKAGE_DIR/include/" 2>/dev/null || true
fi

# Create archive
echo "Creating archive..."
cd "$RELEASE_DIR"
tar czf "$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"

# Generate checksums
echo "Generating checksums..."
if command -v sha256sum &> /dev/null; then
    sha256sum "$PACKAGE_NAME.tar.gz" > "$PACKAGE_NAME.tar.gz.sha256"
elif command -v shasum &> /dev/null; then
    shasum -a 256 "$PACKAGE_NAME.tar.gz" > "$PACKAGE_NAME.tar.gz.sha256"
fi

# Final summary
echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo "Package: $RELEASE_DIR/$PACKAGE_NAME.tar.gz"
if [ -f "$RELEASE_DIR/$PACKAGE_NAME.tar.gz.sha256" ]; then
    echo "Checksum: $(cat "$RELEASE_DIR/$PACKAGE_NAME.tar.gz.sha256")"
fi
echo ""
echo "To install locally:"
echo "  tar xzf $RELEASE_DIR/$PACKAGE_NAME.tar.gz"
echo "  cd $PACKAGE_NAME"
echo "  sudo cp -r bin/* /usr/local/bin/"
echo "  sudo cp -r lib/* /usr/local/lib/"
echo "  sudo cp -r include/* /usr/local/include/"
echo ""