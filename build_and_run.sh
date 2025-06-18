#!/bin/bash

# Simple build and run script for Neo C++ node
echo "================================================"
echo "Neo C++ Node Build and Connectivity Test"
echo "================================================"

# Navigate to project root
cd /home/neo/git/csahrp_cpp

echo "Building Neo C++ node with CMake..."

# Create build directory if it doesn't exist
mkdir -p build_output
cd build_output

# Configure and build with CMake
echo "Configuring project..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    echo "Trying minimal compilation approach..."
    
    cd ..
    echo "Compiling simple connectivity test directly..."
    
    # Try to compile just the basic test
    g++ -std=c++20 -I./include -I./include/neo \
        -pthread -lssl -lcrypto -lboost_system -lboost_filesystem \
        -o simple_node_test test_node_connectivity.cpp \
        2>&1 | head -20
        
    if [ $? -eq 0 ]; then
        echo "✅ Compilation successful"
        echo "Running connectivity test..."
        ./simple_node_test
    else
        echo "❌ Compilation failed"
        echo "Checking for existing executables..."
        find . -name "*.exe" -o -name "neo-*" | head -10
    fi
else
    echo "Building project..."
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo "✅ Build successful"
        echo "Looking for Neo executables..."
        find . -name "neo-*" -executable
        
        echo "Running Neo node..."
        if [ -f "./neo-node" ]; then
            ./neo-node --config ../config/production_config.json
        elif [ -f "./neo-cli" ]; then
            ./neo-cli --config ../config/production_config.json
        else
            echo "No executable found, listing available files:"
            ls -la
        fi
    else
        echo "❌ Build failed"
    fi
fi