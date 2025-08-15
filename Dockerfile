# Multi-stage Dockerfile for Neo C++ node

# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libssl-dev \
    librocksdb-dev \
    libsnappy-dev \
    liblz4-dev \
    libzstd-dev \
    libbz2-dev \
    ninja-build \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build Neo C++
RUN mkdir build && cd build && \
    cmake .. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DNEO_BUILD_TESTS=OFF \
        -DNEO_BUILD_EXAMPLES=OFF \
        -DNEO_ENABLE_LTO=ON \
        -DNEO_ENABLE_HARDENING=ON \
        -DNEO_USE_ROCKSDB=ON && \
    ninja && \
    ninja install

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-thread1.74.0 \
    libssl3 \
    librocksdb7.7 \
    libsnappy1v5 \
    liblz4-1 \
    libzstd1 \
    libbz2-1.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create neo user
RUN useradd -m -U -s /bin/bash neo

# Copy built binaries and libraries
COPY --from=builder /usr/local/bin/neo-* /usr/local/bin/
COPY --from=builder /usr/local/lib/libneo*.so* /usr/local/lib/
COPY --from=builder /usr/local/include/neo /usr/local/include/neo

# Create necessary directories
RUN mkdir -p /var/lib/neo /etc/neo /var/log/neo && \
    chown -R neo:neo /var/lib/neo /etc/neo /var/log/neo

# Copy default configuration
COPY --chown=neo:neo config/mainnet.json /etc/neo/config.json
COPY --chown=neo:neo config/protocol.json /etc/neo/protocol.json

# Switch to neo user
USER neo

# Set working directory
WORKDIR /var/lib/neo

# Expose ports
EXPOSE 10332 10333 10334 20332 20333 20334

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD neo-cli rpc getblockcount || exit 1

# Entry point with script for network selection
COPY docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["mainnet"]