# MinimalRedis TUI Client

A terminal-based user interface client for MinimalRedis server.

## Features

- Interactive command-line interface
- Command history (up/down arrows)
- Real-time pubsub message display
- Connection status monitoring
- Color-coded output (commands, responses, pubsub messages)
- Keyboard shortcuts for common operations

## Usage

```bash
# Start the MinimalRedis server first
./MinimalRedis

# In another terminal, run the TUI client
./redis-tui-client [host] [port]

# Default connection: localhost:6379
./redis-tui-client
```

## Keyboard Shortcuts

- **F1**: Show help
- **F2**: Connect/Disconnect from server
- **F3**: Clear output
- **F10**: Quit
- **Arrow Keys**: Navigate command history and input
- **Enter**: Execute command
- **Backspace/Delete**: Edit input

## Commands

The client supports all Redis commands that MinimalRedis implements:

```bash
SET key value
GET key
DEL key
KEYS pattern
LPUSH key value
RPOP key
SUBSCRIBE channel
PUBLISH channel message
EVAL script numkeys key... arg...
SCRIPT LOAD script
```

## Building

```bash
cd client
mkdir build && cd build
cmake ..
make
```

## Dependencies

- ncurses (for terminal UI)
- C++20 compatible compiler