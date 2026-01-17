# MinimalRedis Enhanced TUI Client

A comprehensive Terminal User Interface client for MinimalRedis server with advanced monitoring and data visualization capabilities.

## Features

### 🎯 Multi-View Interface
- **Command Interface**: Execute Redis commands with history and auto-completion
- **Data Browser**: Visual key-value browsing with pagination
- **Server Monitor**: Real-time server statistics with trend graphs
- **Connection Management**: Easy connect/disconnect functionality
- **Command History**: Persistent command history tracking

### 📊 Advanced Monitoring
- **Real-time Server Stats**: Memory usage, connections, uptime, commands processed
- **Trend Visualization**: ASCII charts for memory and connection trends
- **Live Data Updates**: Continuous monitoring with refresh capabilities

### 🔍 Data Exploration
- **Key Browser**: Paginated view of all database keys
- **Value Preview**: Truncated display of key values
- **Type Indicators**: Visual representation of data types

### ⚡ Performance Features
- **Non-blocking I/O**: Asynchronous network operations
- **Connection Persistence**: Maintains connection state
- **Error Recovery**: Robust error handling and recovery

## Usage

```bash
# Build the client
cd client-enhanced
mkdir build && cd build
cmake ..
make

# Run the client
./redis-enhanced-tui [host] [port]

# Default connection: localhost:6379
./redis-enhanced-tui
```

## Interface Guide

### Main Menu
```
1. Command Interface  - Execute Redis commands
2. Data Browser       - View and browse keys
3. Server Monitor     - Real-time server stats
4. Connection         - Connect/Disconnect
5. Command History    - View previous commands
0. Exit
```

### Command Interface
- Execute any Redis command (SET, GET, DEL, KEYS, INFO, etc.)
- View formatted responses
- Command history is maintained automatically
- Type 'back' to return to main menu

### Data Browser
- Browse all keys in the database
- Paginated view (20 keys per page)
- Navigate with: n(ext), p(rev), r(efresh), b(ack)

### Server Monitor
- Real-time server information
- Memory usage trends with ASCII charts
- Connection count trends
- Press Enter to refresh, 'b' to go back

## Supported Commands

The client supports all MinimalRedis server commands:

### Core Commands
- `SET`, `GET`, `DEL`, `EXISTS`, `KEYS`, `TYPE`

### List Operations
- `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LLEN`, `LRANGE`

### Set Operations
- `SADD`, `SREM`, `SISMEMBER`, `SCARD`, `SMEMBERS`

### Hash Operations
- `HSET`, `HGET`, `HDEL`, `HLEN`, `HKEYS`, `HVALS`

### Sorted Set Operations
- `ZADD`, `ZREM`, `ZCARD`, `ZRANGE`, `ZREVRANGE`

### Advanced Features
- `MULTI`, `EXEC`, `DISCARD`, `WATCH`, `UNWATCH`
- `EXPIRE`, `TTL`, `PERSIST`
- `SUBSCRIBE`, `PUBLISH`, `UNSUBSCRIBE`
- `EVAL`, `EVALSHA`, `SCRIPT`
- `INFO`, `SAVE`, `FLUSHDB`

## Architecture

### Components
- **RedisClient**: Network communication with RESP protocol
- **RespParser**: Redis protocol message parsing
- **DataBrowser**: Key-value visualization
- **ServerMonitor**: Real-time statistics display
- **CommandInterface**: Command execution and history

### Technical Features
- **C++20**: Modern C++ with smart pointers and threading
- **POSIX Sockets**: Cross-platform network communication
- **Non-blocking I/O**: Efficient asynchronous operations
- **Thread-safe Design**: Safe concurrent operations

## Dependencies

- **C++20 compatible compiler**
- **POSIX socket libraries** (included in most Unix systems)
- **CMake 3.16+** for building

## Building

```bash
# Navigate to client directory
cd client-enhanced

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run
./redis-enhanced-tui
```

## Comparison with Original Client

| Feature | Original Client | Enhanced Client |
|---------|----------------|-----------------|
| Interface | Basic ncurses | Multi-view TUI |
| Data Browsing | Text-only | Paginated visual |
| Monitoring | None | Real-time graphs |
| History | File-based | In-memory |
| Auto-completion | Basic | Command-aware |
| Charts | None | ASCII trend graphs |
| Views | Single | 5 specialized views |

## Future Enhancements

- **FTXUI Integration**: Modern component-based UI
- **Script Editor**: In-terminal Lua script editing
- **Connection Profiles**: Save multiple server configurations
- **Export/Import**: Command sequence management
- **Advanced Filtering**: Database subset browsing
- **Performance Metrics**: Detailed timing and throughput stats