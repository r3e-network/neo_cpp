#!/bin/bash

# Neo C++ Docker Build Script
# This script builds Docker images for Neo C++ nodes

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOCKER_REGISTRY=${DOCKER_REGISTRY:-}
IMAGE_TAG=${IMAGE_TAG:-latest}
BUILD_TYPE=${BUILD_TYPE:-Release}

echo -e "${BLUE}Neo C++ Docker Build Script${NC}"
echo "====================================="

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

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed or not in PATH"
    exit 1
fi

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the neo-cpp root directory."
    exit 1
fi

# Function to build Docker image
build_image() {
    local image_name=$1
    local dockerfile=$2
    local context=${3:-.}
    
    print_status "Building Docker image: $image_name"
    
    # Add registry prefix if specified
    if [ -n "$DOCKER_REGISTRY" ]; then
        full_image_name="$DOCKER_REGISTRY/$image_name:$IMAGE_TAG"
    else
        full_image_name="$image_name:$IMAGE_TAG"
    fi
    
    # Build the image
    docker build \
        --build-arg BUILD_TYPE="$BUILD_TYPE" \
        -t "$full_image_name" \
        -f "$dockerfile" \
        "$context"
    
    if [ $? -eq 0 ]; then
        print_status "Successfully built: $full_image_name"
        echo "$full_image_name" >> .docker_images_built
    else
        print_error "Failed to build: $full_image_name"
        return 1
    fi
}

# Create Dockerfiles if they don't exist
mkdir -p docker

# Production Dockerfile
if [ ! -f "docker/Dockerfile.production" ]; then
    print_status "Creating production Dockerfile"
    cat > docker/Dockerfile.production << 'EOF'
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libboost-all-dev \
    libjsoncpp-dev \
    librocksdb-dev \
    libfmt-dev \
    libspdlog-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /neo-cpp

# Copy source code
COPY . .

# Build arguments
ARG BUILD_TYPE=Release

# Build the project
RUN mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DNEO_BUILD_TESTS=OFF \
        -DNEO_BUILD_EXAMPLES=OFF \
        -DNEO_BUILD_TOOLS=ON \
        -DNEO_BUILD_APPS=ON \
        -DNEO_PRODUCTION_BUILD=ON && \
    make -j$(nproc) && \
    make install DESTDIR=/neo-install

# Production image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-thread1.74.0 \
    libjsoncpp25 \
    librocksdb6.11 \
    libfmt8 \
    libspdlog1.10 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create neo user
RUN useradd -m -s /bin/bash neo

# Copy built binaries
COPY --from=builder /neo-install /
COPY --from=builder /neo-cpp/config /opt/neo/config

# Set up directories
RUN mkdir -p /opt/neo/{data,logs} && \
    chown -R neo:neo /opt/neo

# Switch to neo user
USER neo
WORKDIR /opt/neo

# Expose ports
EXPOSE 10333 10332 9090

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:10332/health || exit 1

# Default command
CMD ["neo_node", "/opt/neo/config/production_config.json"]
EOF
fi

# Development Dockerfile
if [ ! -f "docker/Dockerfile.development" ]; then
    print_status "Creating development Dockerfile"
    cat > docker/Dockerfile.development << 'EOF'
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gdb \
    valgrind \
    libssl-dev \
    libboost-all-dev \
    libjsoncpp-dev \
    librocksdb-dev \
    libfmt-dev \
    libspdlog-dev \
    pkg-config \
    vim \
    nano \
    htop \
    && rm -rf /var/lib/apt/lists/*

# Create neo user
RUN useradd -m -s /bin/bash neo

# Set working directory
WORKDIR /neo-cpp

# Copy source code
COPY . .

# Build arguments
ARG BUILD_TYPE=Debug

# Build the project
RUN mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DNEO_BUILD_TESTS=ON \
        -DNEO_BUILD_EXAMPLES=ON \
        -DNEO_BUILD_TOOLS=ON \
        -DNEO_BUILD_APPS=ON \
        -DNEO_ENABLE_SANITIZERS=ON && \
    make -j$(nproc)

# Set up directories
RUN mkdir -p /neo-cpp/{data,logs} && \
    chown -R neo:neo /neo-cpp

# Switch to neo user
USER neo

# Expose ports
EXPOSE 10333 10332 9090

# Default command
CMD ["/bin/bash"]
EOF
fi

# Simple node Dockerfile
if [ ! -f "docker/Dockerfile.simple" ]; then
    print_status "Creating simple node Dockerfile"
    cat > docker/Dockerfile.simple << 'EOF'
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libboost-all-dev \
    libjsoncpp-dev \
    libfmt-dev \
    libspdlog-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /neo-cpp

# Copy source code
COPY . .

# Build arguments
ARG BUILD_TYPE=Release

# Build the project
RUN mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DNEO_BUILD_TESTS=OFF \
        -DNEO_BUILD_EXAMPLES=OFF \
        -DNEO_BUILD_TOOLS=OFF \
        -DNEO_BUILD_APPS=ON && \
    make -j$(nproc) simple_neo_node

# Runtime image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libjsoncpp25 \
    libfmt8 \
    libspdlog1.10 \
    && rm -rf /var/lib/apt/lists/*

# Create neo user
RUN useradd -m -s /bin/bash neo

# Copy built binary
COPY --from=builder /neo-cpp/build/apps/simple_neo_node /usr/local/bin/

# Switch to neo user
USER neo
WORKDIR /home/neo

# Default command
CMD ["simple_neo_node"]
EOF
fi

# Clean up previous build artifacts
rm -f .docker_images_built

# Build images
print_status "Building Docker images..."

build_image "neo-cpp-production" "docker/Dockerfile.production"
build_image "neo-cpp-development" "docker/Dockerfile.development"
build_image "neo-cpp-simple" "docker/Dockerfile.simple"

# Create docker-compose file
print_status "Creating docker-compose.yml"
cat > docker-compose.yml << 'EOF'
version: '3.8'

services:
  neo-mainnet:
    image: neo-cpp-production:latest
    container_name: neo-mainnet
    restart: unless-stopped
    ports:
      - "10333:10333"  # P2P
      - "10332:10332"  # RPC
      - "9090:9090"    # Metrics
    volumes:
      - ./data/mainnet:/opt/neo/data
      - ./logs/mainnet:/opt/neo/logs
      - ./config:/opt/neo/config
    environment:
      - NEO_NETWORK=mainnet
    ulimits:
      nofile:
        soft: 65536
        hard: 65536
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:10332/health"]
      interval: 30s
      timeout: 10s
      retries: 3

  neo-testnet:
    image: neo-cpp-production:latest
    container_name: neo-testnet
    restart: unless-stopped
    ports:
      - "20333:10333"  # P2P
      - "20332:10332"  # RPC
      - "9091:9090"    # Metrics
    volumes:
      - ./data/testnet:/opt/neo/data
      - ./logs/testnet:/opt/neo/logs
      - ./config:/opt/neo/config
    environment:
      - NEO_NETWORK=testnet
    ulimits:
      nofile:
        soft: 65536
        hard: 65536

  neo-simple:
    image: neo-cpp-simple:latest
    container_name: neo-simple
    restart: unless-stopped
    stdin_open: true
    tty: true

  prometheus:
    image: prom/prometheus:latest
    container_name: neo-prometheus
    ports:
      - "9092:9090"
    volumes:
      - ./monitoring/prometheus.yml:/etc/prometheus/prometheus.yml
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
      - '--web.console.libraries=/etc/prometheus/console_libraries'
      - '--web.console.templates=/etc/prometheus/consoles'

  grafana:
    image: grafana/grafana:latest
    container_name: neo-grafana
    ports:
      - "3000:3000"
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    volumes:
      - grafana-storage:/var/lib/grafana

volumes:
  grafana-storage:

networks:
  default:
    driver: bridge
EOF

# Create monitoring configuration
mkdir -p monitoring
if [ ! -f "monitoring/prometheus.yml" ]; then
    cat > monitoring/prometheus.yml << 'EOF'
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'neo-mainnet'
    static_configs:
      - targets: ['neo-mainnet:9090']
  
  - job_name: 'neo-testnet'
    static_configs:
      - targets: ['neo-testnet:9090']
EOF
fi

# Create quick start script
cat > scripts/start_docker.sh << 'EOF'
#!/bin/bash

echo "Starting Neo C++ Docker environment..."

# Create directories
mkdir -p data/{mainnet,testnet} logs/{mainnet,testnet}

# Start services
docker-compose up -d

echo "Services started:"
echo "  Neo Mainnet: http://localhost:10332"
echo "  Neo Testnet: http://localhost:20332"
echo "  Prometheus:  http://localhost:9092"
echo "  Grafana:     http://localhost:3000 (admin/admin)"
echo ""
echo "To stop: docker-compose down"
echo "To view logs: docker-compose logs -f"
EOF

chmod +x scripts/start_docker.sh

# Summary
echo ""
print_status "Docker build completed successfully!"

if [ -f ".docker_images_built" ]; then
    echo -e "${BLUE}Built images:${NC}"
    while read -r image; do
        echo "  - $image"
    done < .docker_images_built
    rm -f .docker_images_built
fi

echo ""
echo -e "${BLUE}Available commands:${NC}"
echo "  Start services:    ./scripts/start_docker.sh"
echo "  Stop services:     docker-compose down"
echo "  View logs:         docker-compose logs -f"
echo "  Simple node:       docker run -it neo-cpp-simple:$IMAGE_TAG"
echo "  Development:       docker run -it neo-cpp-development:$IMAGE_TAG"
echo ""

if [ -n "$DOCKER_REGISTRY" ]; then
    echo -e "${YELLOW}To push images to registry:${NC}"
    echo "  docker push $DOCKER_REGISTRY/neo-cpp-production:$IMAGE_TAG"
    echo "  docker push $DOCKER_REGISTRY/neo-cpp-development:$IMAGE_TAG"
    echo "  docker push $DOCKER_REGISTRY/neo-cpp-simple:$IMAGE_TAG"
    echo ""
fi