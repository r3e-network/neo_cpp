#!/bin/bash

# Neo C++ Docker Validation Script
# Validates Docker builds for mainnet and testnet deployments

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}       Neo C++ Docker Validation Script${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo -e "${RED}✗ Docker is not installed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Docker is installed${NC}"
docker --version

# Check if Dockerfile exists
if [ ! -f "Dockerfile" ]; then
    echo -e "${YELLOW}! Dockerfile not found, creating one...${NC}"
    cat > Dockerfile << 'EOF'
# Neo C++ Docker Image
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libboost-all-dev \
    librocksdb-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    libgtest-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /neo-cpp

# Copy source code
COPY . .

# Build the project
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Runtime image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-thread1.74.0 \
    librocksdb6.11 \
    && rm -rf /var/lib/apt/lists/*

# Create neo user
RUN useradd -m -s /bin/bash neo

# Copy built binaries
COPY --from=builder /neo-cpp/build/apps/neo_node /usr/local/bin/neo_node
COPY --from=builder /neo-cpp/config /home/neo/config

# Set ownership
RUN chown -R neo:neo /home/neo

# Switch to neo user
USER neo
WORKDIR /home/neo

# Expose ports
# MainNet
EXPOSE 10332 10333 10334
# TestNet  
EXPOSE 20332 20333 20334

# Set default command
ENTRYPOINT ["/usr/local/bin/neo_node"]
CMD ["--help"]
EOF
    echo -e "${GREEN}✓ Dockerfile created${NC}"
fi

# Function to test Docker build
test_docker_build() {
    local tag=$1
    echo -e "\n${YELLOW}Building Docker image: ${tag}...${NC}"
    
    if docker build -t ${tag} . > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Docker build successful: ${tag}${NC}"
        return 0
    else
        echo -e "${RED}✗ Docker build failed: ${tag}${NC}"
        return 1
    fi
}

# Function to test Docker run
test_docker_run() {
    local tag=$1
    local network=$2
    local port_base=$3
    
    echo -e "\n${YELLOW}Testing ${network} deployment...${NC}"
    
    # Stop any existing container
    docker stop neo-test-${network} 2>/dev/null || true
    docker rm neo-test-${network} 2>/dev/null || true
    
    # Run container
    echo -e "  Starting container..."
    if docker run -d \
        --name neo-test-${network} \
        -p ${port_base}32:${port_base}32 \
        -p ${port_base}33:${port_base}33 \
        -p ${port_base}34:${port_base}34 \
        ${tag} \
        --network ${network} \
        --log-level info > /dev/null 2>&1; then
        
        echo -e "  ${GREEN}✓ Container started${NC}"
        
        # Wait for container to initialize
        sleep 5
        
        # Check if container is still running
        if docker ps | grep -q neo-test-${network}; then
            echo -e "  ${GREEN}✓ Container is running${NC}"
            
            # Check logs
            if docker logs neo-test-${network} 2>&1 | grep -q "error\|Error\|ERROR"; then
                echo -e "  ${YELLOW}! Errors found in logs${NC}"
                docker logs neo-test-${network} 2>&1 | grep -i error | head -5
            else
                echo -e "  ${GREEN}✓ No errors in logs${NC}"
            fi
            
            # Stop container
            docker stop neo-test-${network} > /dev/null 2>&1
            docker rm neo-test-${network} > /dev/null 2>&1
            return 0
        else
            echo -e "  ${RED}✗ Container crashed${NC}"
            docker logs neo-test-${network} 2>&1 | tail -10
            docker rm neo-test-${network} > /dev/null 2>&1
            return 1
        fi
    else
        echo -e "  ${RED}✗ Failed to start container${NC}"
        return 1
    fi
}

# Build Docker images
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Building Docker Images${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

test_docker_build "neo-cpp:latest"
test_docker_build "neo-cpp:mainnet"
test_docker_build "neo-cpp:testnet"

# Test Docker deployments
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Testing Docker Deployments${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

# Test mainnet
test_docker_run "neo-cpp:mainnet" "mainnet" "103"

# Test testnet
test_docker_run "neo-cpp:testnet" "testnet" "203"

# Summary
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Validation Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

echo -e "\n${GREEN}Docker validation complete!${NC}"
echo -e "\nTo run manually:"
echo -e "  ${YELLOW}make docker${NC}              - Build Docker image"
echo -e "  ${YELLOW}make run-docker-mainnet${NC}  - Run mainnet in Docker"
echo -e "  ${YELLOW}make run-docker-testnet${NC}  - Run testnet in Docker"