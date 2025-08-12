#!/bin/bash

# Docker build script with retry logic and registry fallback
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Neo C++ Docker Build Script${NC}"
echo "================================"

# Configuration
IMAGE_NAME="neo-cpp-node"
IMAGE_TAG="${1:-latest}"
DOCKERFILE="${2:-Dockerfile.optimized}"
BUILD_ARGS="${3:-}"

# Function to check Docker daemon
check_docker() {
    if ! docker info >/dev/null 2>&1; then
        echo -e "${RED}Error: Docker daemon is not running${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Docker daemon is running${NC}"
}

# Function to pull base image with retry
pull_base_image() {
    local image="ubuntu:22.04"
    local max_retries=3
    local retry_delay=5
    
    echo -e "${BLUE}Pulling base image: $image${NC}"
    
    for i in $(seq 1 $max_retries); do
        echo "Attempt $i of $max_retries..."
        
        if docker pull $image; then
            echo -e "${GREEN}✓ Base image pulled successfully${NC}"
            return 0
        fi
        
        if [ $i -lt $max_retries ]; then
            echo -e "${YELLOW}Failed to pull image, retrying in ${retry_delay}s...${NC}"
            sleep $retry_delay
        fi
    done
    
    echo -e "${RED}Failed to pull base image after $max_retries attempts${NC}"
    return 1
}

# Function to build Docker image
build_image() {
    echo -e "${BLUE}Building Docker image...${NC}"
    
    # Build with BuildKit for better caching
    export DOCKER_BUILDKIT=1
    
    if docker build \
        --progress=plain \
        --tag "${IMAGE_NAME}:${IMAGE_TAG}" \
        --file "${DOCKERFILE}" \
        ${BUILD_ARGS} \
        .; then
        echo -e "${GREEN}✓ Docker image built successfully${NC}"
        return 0
    else
        echo -e "${RED}Failed to build Docker image${NC}"
        return 1
    fi
}

# Function to verify image
verify_image() {
    echo -e "${BLUE}Verifying Docker image...${NC}"
    
    # Check if image exists
    if docker images | grep -q "${IMAGE_NAME}.*${IMAGE_TAG}"; then
        echo -e "${GREEN}✓ Image exists${NC}"
    else
        echo -e "${RED}Image not found${NC}"
        return 1
    fi
    
    # Test the image
    echo "Testing image..."
    if docker run --rm "${IMAGE_NAME}:${IMAGE_TAG}" neo_node --version; then
        echo -e "${GREEN}✓ Image runs successfully${NC}"
    else
        echo -e "${RED}Image test failed${NC}"
        return 1
    fi
    
    # Show image details
    echo -e "\n${BLUE}Image Details:${NC}"
    docker images "${IMAGE_NAME}:${IMAGE_TAG}"
    
    return 0
}

# Main execution
main() {
    echo "Starting Docker build process..."
    echo ""
    
    # Step 1: Check Docker
    check_docker || exit 1
    echo ""
    
    # Step 2: Pull base image
    if ! pull_base_image; then
        echo -e "${YELLOW}Warning: Could not pull base image, trying local build anyway...${NC}"
    fi
    echo ""
    
    # Step 3: Build image
    build_image || exit 1
    echo ""
    
    # Step 4: Verify image
    verify_image || exit 1
    echo ""
    
    echo -e "${GREEN}================================${NC}"
    echo -e "${GREEN}Docker build completed successfully!${NC}"
    echo -e "${GREEN}================================${NC}"
    echo ""
    echo "To run the container:"
    echo "  Mainnet: docker run -d -p 30333:30333 -p 30332:30332 ${IMAGE_NAME}:${IMAGE_TAG}"
    echo "  Testnet: docker run -d -p 30333:30333 -p 30332:30332 ${IMAGE_NAME}:${IMAGE_TAG} --config /opt/neo/config/testnet.json"
    echo ""
}

# Run main function
main