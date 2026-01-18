# MinimalRedis Production Deployment Guide

This guide covers deploying MinimalRedis in production environments.

## System Requirements

### Minimum Hardware
- **CPU**: 1 core (2+ recommended for concurrent workloads)
- **RAM**: 256MB minimum (1GB+ recommended)
- **Storage**: 100MB for binaries + data storage
- **Network**: 1Gbps Ethernet recommended

### Software Dependencies
- **Operating System**: Linux, macOS, or Windows
- **Compiler**: C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **Build Tools**: CMake 3.16+, Make or Ninja
- **Libraries**:
  - Lua 5.4 (for scripting)
  - POSIX threads (built-in on Unix systems)
  - Curses library (for TUI clients)

## Building for Production

### Optimized Release Build

```bash
# Create release build directory
mkdir build-release && cd build-release

# Configure with optimization flags
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -flto" \
      -DCMAKE_EXE_LINKER_FLAGS="-flto" \
      ..

# Build with parallel jobs
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### Static Linking (Optional)

For deployment without external dependencies:

```bash
# Install static versions of dependencies
# On Ubuntu/Debian:
sudo apt-get install liblua5.4-dev libncurses5-dev

# Configure with static linking
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -static-libgcc -static-libstdc++" \
      -DLUA_LIBRARIES="/usr/lib/x86_64-linux-gnu/liblua5.4.a" \
      ..
```

## Deployment Options

### 1. Standalone Server

```bash
# Copy binary to production
scp MinimalRedis user@server:/opt/minimalredis/

# Create data directory
ssh user@server "mkdir -p /var/lib/minimalredis"

# Run server
ssh user@server "/opt/minimalredis/MinimalRedis"
```

### 2. Systemd Service

Create `/etc/systemd/system/minimalredis.service`:

```ini
[Unit]
Description=MinimalRedis Server
After=network.target

[Service]
Type=simple
User=redis
Group=redis
ExecStart=/opt/minimalredis/MinimalRedis
WorkingDirectory=/var/lib/minimalredis
Restart=always
RestartSec=5
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable minimalredis
sudo systemctl start minimalredis
sudo systemctl status minimalredis
```

### 3. Docker Container

Create `Dockerfile`:

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    liblua5.4-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /src
WORKDIR /src

RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    liblua5.4-0 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /bin/false redis

COPY --from=0 /src/build/MinimalRedis /usr/local/bin/
COPY --from=0 /src/build/MinimalRedisTests /usr/local/bin/

USER redis
EXPOSE 6379

CMD ["/usr/local/bin/MinimalRedis"]
```

Build and run:

```bash
docker build -t minimalredis .
docker run -p 6379:6379 -v /data:/data minimalredis
```

## Configuration

### Environment Variables

```bash
# Set data directory
export MINIMALREDIS_DATA_DIR=/var/lib/minimalredis

# Set log level (future feature)
export MINIMALREDIS_LOG_LEVEL=info

# Set max connections (future feature)
export MINIMALREDIS_MAX_CONNECTIONS=10000
```

### Data Persistence

MinimalRedis automatically saves data to `minimalredis.db` in the working directory.

- **Auto-save**: Every 30 seconds if data has changed
- **Manual save**: Use `SAVE` command
- **Background save**: Use `BGSAVE` command

### Memory Management

- **Automatic cleanup**: Expired keys are removed on access
- **Memory pooling**: Efficient allocation for frequent operations
- **RAII patterns**: Automatic resource cleanup

## Monitoring

### Built-in Monitoring

```bash
# Connect with redis-cli
redis-cli -p 6379

# Get server info
INFO

# Monitor commands (in another terminal)
MONITOR
```

### Key Metrics to Monitor

- **Memory usage**: `INFO` command shows memory statistics
- **Connection count**: `INFO` shows active connections
- **Command throughput**: Commands processed per second
- **Key count**: Total keys in database (`DBSIZE`)

### Log Files

MinimalRedis currently logs to stdout/stderr. For production:

```bash
# Systemd service logs
journalctl -u minimalredis -f

# Docker logs
docker logs -f minimalredis-container
```

## Security Considerations

### Network Security

- **Firewall**: Restrict port 6379 to trusted networks
- **TLS**: Not currently supported (consider reverse proxy)
- **Authentication**: Not implemented (use network restrictions)

### Data Security

- **File permissions**: Restrict access to data files
- **Backup**: Regular backups of `minimalredis.db`
- **Encryption**: Not built-in (encrypt at filesystem level)

### Best Practices

```bash
# Create dedicated user
sudo useradd -r -s /bin/false redis

# Set proper permissions
sudo chown redis:redis /opt/minimalredis/MinimalRedis
sudo chown redis:redis /var/lib/minimalredis
sudo chmod 700 /var/lib/minimalredis

# Enable core dumps for debugging
sudo systemctl set-property minimalredis DumpCore=yes
```

## Performance Tuning

### Connection Limits

- **File descriptors**: Increase system limits
  ```bash
  echo "redis soft nofile 65536" | sudo tee -a /etc/security/limits.d/redis.conf
  echo "redis hard nofile 65536" | sudo tee -a /etc/security/limits.d/redis.conf
  ```

### Memory Tuning

- **Transparent huge pages**: May hurt performance
  ```bash
  echo never | sudo tee /sys/kernel/mm/transparent_hugepage/enabled
  ```

### Network Tuning

- **TCP keepalive**: Prevent stale connections
- **Buffer sizes**: Tune for high-throughput workloads

## Backup and Recovery

### Automated Backups

```bash
#!/bin/bash
# Daily backup script
DATE=$(date +%Y%m%d_%H%M%S)
cp /var/lib/minimalredis/minimalredis.db /backup/minimalredis_$DATE.db

# Keep last 7 days
find /backup -name "minimalredis_*.db" -mtime +7 -delete
```

### Recovery

```bash
# Stop server
sudo systemctl stop minimalredis

# Restore from backup
cp /backup/minimalredis_20241201_020000.db /var/lib/minimalredis/minimalredis.db

# Start server
sudo systemctl start minimalredis
```

## Troubleshooting

### Common Issues

1. **Port already in use**
   ```bash
   sudo netstat -tulpn | grep :6379
   sudo kill -9 <PID>
   ```

2. **Permission denied**
   ```bash
   sudo chown redis:redis /var/lib/minimalredis
   sudo chmod 700 /var/lib/minimalredis
   ```

3. **Out of memory**
   - Monitor memory usage with `INFO`
   - Increase system memory
   - Check for memory leaks

### Debug Mode

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Run with verbose logging (future feature)
./MinimalRedis --verbose
```

## High Availability (Future)

While MinimalRedis doesn't currently support clustering, consider:

- **Load balancing**: HAProxy or nginx for multiple instances
- **Replication**: Manual synchronization (future feature)
- **Backup instances**: Hot standby servers

## Support

For issues and questions:
- Check the [README.md](../README.md) for basic usage
- Review [test cases](../tests/) for expected behavior
- File issues on the GitHub repository

---

**MinimalRedis is production-ready for many use cases, especially development, testing, and moderate production workloads.** 🚀