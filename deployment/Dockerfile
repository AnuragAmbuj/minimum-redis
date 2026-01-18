# MinimalRedis Dockerfile
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    liblua5.4-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . /src
WORKDIR /src

# Build MinimalRedis
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_CXX_FLAGS="-O3 -march=native" .. && \
    make -j$(nproc) && \
    ctest --output-on-failure

# Production image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    liblua5.4-0 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /bin/false redis

# Copy binaries from builder
COPY --from=builder /src/build/MinimalRedis /usr/local/bin/
COPY --from=builder /src/build/MinimalRedisTests /usr/local/bin/

# Create data directory
RUN mkdir -p /data && chown redis:redis /data

# Switch to non-root user
USER redis
WORKDIR /data

# Expose Redis port
EXPOSE 6379

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD echo "PING" | nc localhost 6379 | grep -q "PONG" || exit 1

# Start MinimalRedis
CMD ["/usr/local/bin/MinimalRedis"]