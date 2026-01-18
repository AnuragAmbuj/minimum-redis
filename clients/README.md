# MinimalRedis Unified TUI Client

MinimalRedis provides a comprehensive Terminal User Interface (TUI) client that combines all features for interacting with the Redis server.

## Unified TUI Client (`tui/`)

A feature-rich client with both basic and advanced Redis operations.

**Features:**
- **Multi-view interface**: Command execution, Data Browser, Server Monitor, Connection management, Command history
- **Real-time server statistics** with trend graphs and monitoring
- **Paginated data browser** for key-value pairs with visual representation
- **Command-line interface** with auto-completion and history
- **Advanced monitoring dashboard** with live updates
- **All Redis commands** supported (SET, GET, DEL, KEYS, INFO, transactions, etc.)

**Use Case:** Complete Redis development, testing, monitoring, and production management.

**Build & Run:**
```bash
# Using the unified build script (recommended)
./build.sh

# Or manually:
cd clients/tui
mkdir build && cd build
cmake ..
make
./redis-tui
```

## Interface Guide

### Main Menu
```
1. Command Interface  - Execute Redis commands with auto-completion
2. Data Browser       - Visual key-value browsing with pagination
3. Server Monitor     - Real-time server stats with trend graphs
4. Connection         - Connect/Disconnect from server
5. Command History    - View previous commands
0. Exit
```

### Key Features
- **Command Interface**: Execute any Redis command with history and auto-completion
- **Data Browser**: Paginated view of all keys with type indicators
- **Server Monitor**: Live memory usage, connections, and command throughput graphs
- **Connection Management**: Easy connect/disconnect functionality
- **Command History**: Persistent command tracking across sessions

## Supported Commands

All Redis commands are supported including:
- **Core**: SET, GET, DEL, EXISTS, KEYS, TYPE
- **Lists**: LPUSH, RPUSH, LPOP, RPOP, LLEN, LRANGE, LINDEX
- **Sets**: SADD, SREM, SISMEMBER, SCARD, SMEMBERS
- **Hashes**: HSET, HGET, HDEL, HLEN, HKEYS, HVALS, HGETALL
- **Sorted Sets**: ZADD, ZREM, ZCARD, ZRANGE, ZREVRANGE, ZSCORE, ZRANK
- **Advanced**: MULTI, EXEC, DISCARD, WATCH, UNWATCH, EXPIRE, TTL, PUBLISH, SUBSCRIBE
- **Server**: INFO, SAVE, FLUSHDB, DBSIZE

## Architecture

The unified client combines the best of both previous implementations:
- **Simple command interface** for quick operations
- **Advanced monitoring** for production use
- **Comprehensive data exploration** tools
- **Professional UI** with multiple specialized views

This single client provides everything needed for Redis development and operations!