#!/bin/bash
# Setup script for vcpkg dependency manager

set -e

echo "Setting up vcpkg for Neo C++ project..."

# Clone vcpkg if not present
if [ ! -d "vcpkg" ]; then
    echo "Cloning vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    cd ..
else
    echo "vcpkg already exists, updating..."
    cd vcpkg
    git pull
    ./bootstrap-vcpkg.sh
    cd ..
fi

# Install dependencies
echo "Installing dependencies..."
./vcpkg/vcpkg install \
    openssl \
    nlohmann-json \
    spdlog \
    gtest \
    benchmark \
    rocksdb \
    leveldb \
    boost-system \
    boost-filesystem \
    boost-thread \
    boost-asio \
    protobuf \
    grpc \
    fmt \
    cpprestsdk

echo "vcpkg setup complete!"
echo "To use vcpkg with CMake, add: -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake"