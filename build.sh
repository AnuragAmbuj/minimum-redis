#!/bin/bash

# MinimalRedis Build Script
# Checks dependencies and builds server and clients

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if command exists
check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Check for required tools
check_dependencies() {
    log_info "Checking build dependencies..."

    local missing_deps=()

    # Check for C++ compiler
    if ! check_command g++ && ! check_command clang++; then
        missing_deps+=("C++ compiler (g++ or clang++)")
    else
        log_success "C++ compiler found"
    fi

    # Check for CMake
    if ! check_command cmake; then
        missing_deps+=("CMake")
    else
        local cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
        if [[ "$(printf '%s\n' "$cmake_version" "3.16" | sort -V | head -n1)" != "3.16" ]]; then
            log_warning "CMake version $cmake_version found, but 3.16+ recommended"
        else
            log_success "CMake $cmake_version found"
        fi
    fi

    # Check for Make
    if ! check_command make; then
        missing_deps+=("Make")
    else
        log_success "Make found"
    fi

    # Check for pkg-config (for library detection)
    if ! check_command pkg-config; then
        log_warning "pkg-config not found - library detection may be limited"
    else
        log_success "pkg-config found"
    fi

    # Detect platform for additional checks
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        log_info "Detected Linux platform"
        # Additional Linux-specific checks could go here
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        log_info "Detected macOS platform"
        # Additional macOS-specific checks could go here
    else
        log_warning "Unknown platform: $OSTYPE - compatibility not guaranteed"
    fi

    # Check for Lua
    if ! pkg-config --exists lua5.4 2>/dev/null && ! pkg-config --exists lua 2>/dev/null; then
        # Try to find lua.h directly
        if [[ ! -f "/usr/include/lua5.4/lua.h" && ! -f "/usr/local/include/lua5.4/lua.h" && ! -f "/opt/homebrew/include/lua5.4/lua.h" ]]; then
            missing_deps+=("Lua 5.4 development headers")
        else
            log_success "Lua headers found"
        fi
    else
        log_success "Lua found via pkg-config"
    fi

    # Check for Curses (for TUI clients)
    if ! pkg-config --exists ncurses 2>/dev/null && ! pkg-config --exists curses 2>/dev/null; then
        if [[ ! -f "/usr/include/curses.h" && ! -f "/usr/include/ncurses.h" ]]; then
            missing_deps+=("Curses library (ncurses)")
        else
            log_success "Curses headers found"
        fi
    else
        log_success "Curses library found"
    fi

    # Check for Google Test (optional, for tests)
    if ! pkg-config --exists gtest 2>/dev/null; then
        log_warning "Google Test not found - tests will not be built"
    else
        log_success "Google Test found"
    fi

    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_error "Missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo
        log_info "Please install missing dependencies and try again."
        echo "On Ubuntu/Debian: sudo apt-get install build-essential cmake liblua5.4-dev libncurses5-dev libgtest-dev"
        echo "On macOS: brew install cmake lua ncurses googletest"
        exit 1
    fi

    log_success "All required dependencies found!"
}

# Create build directory
setup_build_dir() {
    local build_dir="$1"
    if [[ ! -d "$build_dir" ]]; then
        mkdir -p "$build_dir"
        log_info "Created build directory: $build_dir"
    fi
}

# Build component
build_component() {
    local name="$1"
    local source_dir="$2"
    local build_dir="$3"
    local executable_name="$4"

    log_info "Building $name..."

    setup_build_dir "$build_dir"

    cd "$build_dir"

    # Configure with CMake
    log_info "Configuring $name with CMake..."
    if ! cmake "$source_dir" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_CXX_FLAGS="-O3 -march=native" \
        2>/dev/null; then
        log_error "CMake configuration failed for $name"
        return 1
    fi

    # Build
    log_info "Building $name with $JOBS jobs..."
    if ! make -j"$JOBS" 2>/dev/null; then
        log_error "Build failed for $name"
        return 1
    fi

    # Check if executable exists
    if [[ -f "$executable_name" ]]; then
        log_success "$name built successfully: $build_dir/$executable_name"
    else
        log_error "Executable not found: $executable_name"
        return 1
    fi

    cd "$PROJECT_ROOT"
    return 0
}

# Build server
build_server() {
    log_info "Building MinimalRedis Server..."
    build_component "Server" "$PROJECT_ROOT" "$PROJECT_ROOT/build-server" "MinimalRedis"
}

# Build unified TUI client
build_tui_client() {
    log_info "Building Unified TUI Client..."
    build_component "Unified TUI Client" "$PROJECT_ROOT/clients/tui" "$PROJECT_ROOT/clients/tui/build" "redis-tui"
}

# Main script
main() {
    log_info "MinimalRedis Build Script"
    log_info "Build type: $BUILD_TYPE"
    log_info "Parallel jobs: $JOBS"
    echo

    # Change to project root
    cd "$PROJECT_ROOT"

    # Check dependencies
    check_dependencies
    echo

    # Run tests first
    log_info "Running test suites..."
    cd "$PROJECT_ROOT"
    if [[ -d "build-server" ]]; then
        cd build-server
        log_info "Running server tests..."
        if ! ./MinimalRedisTests 2>/dev/null; then
            log_error "Server tests failed! Aborting build."
            cd "$PROJECT_ROOT"
            exit 1
        fi
        log_success "Server tests passed!"

        if [[ -f "MinimalRedisClientTests" ]]; then
            log_info "Running client tests..."
            if ! ./MinimalRedisClientTests 2>/dev/null; then
                log_error "Client tests failed! Aborting build."
                cd "$PROJECT_ROOT"
                exit 1
            fi
            log_success "Client tests passed!"
        else
            log_warning "Client test executable not found."
        fi
        cd "$PROJECT_ROOT"
    else
        log_warning "Test executables not found. Building without running tests."
    fi
    log_success "All available tests passed!"

    # Build components
    local build_failed=0

    if ! build_server; then
        build_failed=1
    fi

    if ! build_tui_client; then
        build_failed=1
    fi

    echo
    if [[ $build_failed -eq 0 ]]; then
        log_success "All components built successfully!"
        echo
        log_info "Executables created:"
        echo "  - Server: $PROJECT_ROOT/build-server/MinimalRedis"
        echo "  - Unified TUI Client: $PROJECT_ROOT/clients/tui/build/redis-tui"
        echo
        log_info "To run the server: ./build-server/MinimalRedis"
        log_info "To run the TUI client: ./clients/tui/build/redis-tui"
    else
        log_error "Some builds failed. Please check the errors above."
        exit 1
    fi
}

# Run main function
main "$@"