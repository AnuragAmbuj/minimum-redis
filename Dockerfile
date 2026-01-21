# Multi-Stage Docker Build for MinimalRedis Cluster

# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgtest-dev \
    liblua5.3-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Production stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    liblua5.3-0 \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -r -s /bin/false redis

# Create necessary directories
RUN mkdir -p /app/data /app/logs /app/wal

# Copy binary from builder stage
COPY --from=builder /app/build/MinimalRedis /app/minimal-redis

# Set ownership
RUN chown -R redis:redis /app

# Switch to redis user
USER redis

# Set working directory
WORKDIR /app

# Expose default port (can be overridden)
EXPOSE 6379

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD /app/minimal-redis --help 2>/dev/null || exit 1