# Production Deployment

## 📦 Enterprise Deployment Solutions

MinimalRedis provides comprehensive deployment options for production environments, from simple standalone installations to complex containerized deployments.

## 🐳 Docker Containerization

### Multi-Stage Production Build
```dockerfile
# Multi-stage Dockerfile for production deployment
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    liblua5.4-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy and build
COPY . /src
WORKDIR /src
RUN ./build.sh

# Production image
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    liblua5.4-0 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /bin/false redis

# Copy built binaries
COPY --from=builder /src/build-server/MinimalRedis /usr/local/bin/
COPY --from=builder /src/clients/tui/build/redis-tui /usr/local/bin/

# Create data directory
RUN mkdir -p /data && chown redis:redis /data

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD echo "PING" | timeout 3 nc localhost 6379 | grep -q "PONG" || exit 1

# Switch to non-root user
USER redis
WORKDIR /data
EXPOSE 6379

CMD ["/usr/local/bin/MinimalRedis"]
```

### Docker Compose Orchestration
```yaml
version: '3.8'

services:
  minimalredis:
    build:
      context: ..
      dockerfile: deployment/Dockerfile
    ports:
      - "6379:6379"
    volumes:
      - redis-data:/data
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "echo", "PING", "|", "nc", "localhost", "6379"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s

  redis-cli:
    image: redis:alpine
    command: ["redis-cli", "-h", "minimalredis", "monitor"]
    depends_on:
      - minimalredis
    profiles:
      - debug

volumes:
  redis-data:
    driver: local
```

## 🐧 Systemd Service Management

### Production Service Configuration
```ini
[Unit]
Description=MinimalRedis High-Performance Data Store
After=network.target
Requires=network.target

[Service]
Type=simple
User=redis
Group=redis
ExecStart=/usr/local/bin/MinimalRedis
WorkingDirectory=/var/lib/minimalredis
Restart=always
RestartSec=5

# Security hardening
NoNewPrivileges=yes
PrivateTmp=yes
ProtectHome=yes
ReadWritePaths=/var/lib/minimalredis

# Performance tuning
LimitNOFILE=65536
LimitNPROC=4096

# Logging
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### Service Management Commands
```bash
# Install service
sudo cp deployment/minimalredis.service /etc/systemd/system/
sudo systemctl daemon-reload

# Enable and start
sudo systemctl enable minimalredis
sudo systemctl start minimalredis

# Monitor service
sudo systemctl status minimalredis
sudo journalctl -u minimalredis -f

# Performance monitoring
sudo systemctl set-property minimalredis CPUQuota=80%
```

## 🏢 Enterprise Installation

### Standalone Production Installation
```bash
# Create dedicated user
sudo useradd -r -s /bin/false redis
sudo mkdir -p /var/lib/minimalredis
sudo chown redis:redis /var/lib/minimalredis

# Install binaries
sudo cp build-server/MinimalRedis /usr/local/bin/
sudo cp clients/tui/build/redis-tui /usr/local/bin/

# Configure permissions
sudo chmod 755 /usr/local/bin/MinimalRedis
sudo chmod 755 /usr/local/bin/redis-tui

# Install systemd service
sudo cp deployment/minimalredis.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable minimalredis
```

### Multi-Instance Deployment
```bash
# Create multiple instances
for instance in {1..3}; do
    sudo useradd -r -s /bin/false redis$instance
    sudo mkdir -p /var/lib/minimalredis/instance$instance
    sudo chown redis$instance:redis$instance /var/lib/minimalredis/instance$instance

    # Create instance-specific service
    cat > /etc/systemd/system/minimalredis$instance.service << EOF
[Unit]
Description=MinimalRedis Instance $instance
After=network.target

[Service]
User=redis$instance
Group=redis$instance
ExecStart=/usr/local/bin/MinimalRedis --port 637$instance
WorkingDirectory=/var/lib/minimalredis/instance$instance
Restart=always

[Install]
WantedBy=multi-user.target
EOF
done

# Enable all instances
sudo systemctl daemon-reload
for instance in {1..3}; do
    sudo systemctl enable minimalredis$instance
    sudo systemctl start minimalredis$instance
done
```

## 🔒 Security Hardening

### Network Security
```bash
# Firewall configuration (Ubuntu/Debian)
sudo ufw allow 6379/tcp
sudo ufw --force enable

# Restrict to specific networks
sudo ufw allow from 192.168.1.0/24 to any port 6379

# SELinux/AppArmor (if applicable)
sudo apparmor_parser -r /etc/apparmor.d/usr.bin.minimalredis
```

### File System Security
```bash
# Secure data directory permissions
sudo chown redis:redis /var/lib/minimalredis
sudo chmod 700 /var/lib/minimalredis

# Secure binary permissions
sudo chown root:root /usr/local/bin/MinimalRedis
sudo chmod 755 /usr/local/bin/MinimalRedis

# Enable core dumps for debugging (controlled)
sudo systemctl set-property minimalredis DumpCore=yes
sudo sysctl -w kernel.core_pattern=/var/lib/minimalredis/core.%e.%p
```

### Process Security
```ini
# systemd service hardening
[Service]
NoNewPrivileges=yes
PrivateTmp=yes
ProtectHome=yes
ProtectSystem=strict
ReadWritePaths=/var/lib/minimalredis
CapabilityBoundingSet=
```

## 📊 Monitoring & Observability

### Built-in Monitoring
```bash
# Connect to running instance
redis-tui localhost 6379

# Monitor commands in real-time
MONITOR

# Server information
INFO

# Memory statistics
INFO memory

# Client connections
INFO clients
```

### External Monitoring Integration
```bash
# Prometheus metrics export (future feature)
curl http://localhost:6379/metrics

# Health check endpoint
curl http://localhost:6379/health

# Custom monitoring script
#!/bin/bash
REDIS_CLI="redis-tui localhost 6379"

# Memory usage alert
MEMORY_USAGE=$($REDIS_CLI INFO memory | grep used_memory: | cut -d: -f2)
if [ $MEMORY_USAGE -gt 1073741824 ]; then  # 1GB
    echo "High memory usage: $MEMORY_USAGE bytes"
fi
```

### Log Management
```bash
# systemd journal integration
sudo journalctl -u minimalredis --since "1 hour ago"

# Log rotation
sudo cat > /etc/logrotate.d/minimalredis << EOF
/var/log/minimalredis/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 redis redis
    postrotate
        systemctl reload minimalredis
    endscript
}
EOF
```

## 🚀 Performance Tuning

### Memory Optimization
```bash
# Configure system for Redis performance
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 1 > /proc/sys/vm/overcommit_memory

# Increase file descriptor limits
echo "redis soft nofile 65536" >> /etc/security/limits.d/redis.conf
echo "redis hard nofile 65536" >> /etc/security/limits.d/redis.conf
```

### Network Optimization
```bash
# TCP tuning for high-throughput
sysctl -w net.core.somaxconn=65536
sysctl -w net.ipv4.tcp_max_syn_backlog=65536
sysctl -w net.core.netdev_max_backlog=65536

# Disable TCP timestamps for lower latency
sysctl -w net.ipv4.tcp_timestamps=0
```

### CPU Affinity (Multi-core systems)
```ini
# Pin Redis to specific CPU cores
[Service]
CPUAffinity=0-7  # Use first 8 CPU cores
```

## 🔄 Backup & Recovery

### Automated Backup Strategy
```bash
#!/bin/bash
# Daily backup script

BACKUP_DIR="/var/backups/minimalredis"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create backup directory
mkdir -p $BACKUP_DIR

# Stop Redis (briefly for consistent backup)
sudo systemctl stop minimalredis

# Copy data files
cp /var/lib/minimalredis/minimalredis.db $BACKUP_DIR/minimalredis_$TIMESTAMP.db

# Restart Redis
sudo systemctl start minimalredis

# Compress backup
gzip $BACKUP_DIR/minimalredis_$TIMESTAMP.db

# Cleanup old backups (keep 7 days)
find $BACKUP_DIR -name "minimalredis_*.db.gz" -mtime +7 -delete

echo "Backup completed: $BACKUP_DIR/minimalredis_$TIMESTAMP.db.gz"
```

### Point-in-Time Recovery
```bash
# Stop Redis
sudo systemctl stop minimalredis

# Restore from backup
BACKUP_FILE=$(ls -t /var/backups/minimalredis/minimalredis_*.db.gz | head -1)
gunzip -c $BACKUP_FILE > /var/lib/minimalredis/minimalredis.db

# Verify permissions
sudo chown redis:redis /var/lib/minimalredis/minimalredis.db

# Restart Redis
sudo systemctl start minimalredis
```

## 📚 Deployment Documentation

### Production Checklist
- [ ] Server capacity planning completed
- [ ] Network security configured
- [ ] Backup strategy implemented
- [ ] Monitoring alerts configured
- [ ] Performance baselines established
- [ ] Failover procedures documented
- [ ] Security hardening applied

### Operational Runbooks
- **Startup Procedures**: Service initialization and verification
- **Shutdown Procedures**: Graceful service termination
- **Maintenance Windows**: Scheduled maintenance procedures
- **Incident Response**: Troubleshooting and recovery procedures
- **Performance Tuning**: Optimization procedures and monitoring

This comprehensive deployment framework ensures MinimalRedis can be reliably deployed in enterprise production environments with proper security, monitoring, and operational procedures.