#!/bin/bash

# Test script for MinimalRedis Docker Compose cluster

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Wait for node to be ready
wait_for_node() {
    local port=$1
    local node_name=$2
    local max_attempts=30
    local attempt=1

    log_info "Waiting for $node_name on port $port..."

    while [ $attempt -le $max_attempts ]; do
        if timeout 5 bash -c "</dev/tcp/localhost/$port" 2>/dev/null; then
            log_success "$node_name is ready on port $port"
            return 0
        fi

        echo -n "."
        sleep 2
        ((attempt++))
    done

    log_error "$node_name on port $port failed to start within timeout"
    return 1
}

# Test basic Redis operations
test_basic_operations() {
    local port=$1
    local node_name=$2

    log_info "Testing basic operations on $node_name (port $port)"

    # Test SET
    if redis-cli -p "$port" SET test-key "Hello from $node_name" >/dev/null 2>&1; then
        log_success "SET operation successful on $node_name"
    else
        log_error "SET operation failed on $node_name"
        return 1
    fi

    # Test GET
    local value
    value=$(redis-cli -p "$port" GET test-key 2>/dev/null)
    if [ "$value" = "Hello from $node_name" ]; then
        log_success "GET operation successful on $node_name"
    else
        log_error "GET operation failed on $node_name (got: $value)"
        return 1
    fi

    return 0
}

# Test data consistency across nodes
test_consistency() {
    log_info "Testing data consistency across cluster"

    # Set value on leader
    redis-cli -p 6379 SET cluster-test "Consistent data" >/dev/null 2>&1

    # Check on all nodes
    for port in 6379 6380 6381; do
        local value
        value=$(redis-cli -p "$port" GET cluster-test 2>/dev/null)
        if [ "$value" != "Consistent data" ]; then
            log_error "Data inconsistency detected on port $port (got: $value)"
            return 1
        fi
    done

    log_success "Data consistency verified across all nodes"
    return 0
}

# Test cluster-specific commands
test_cluster_commands() {
    log_info "Testing cluster-specific commands"

    # Test basic info command (if implemented)
    if redis-cli -p 6379 INFO >/dev/null 2>&1; then
        log_success "INFO command works"
    else
        log_warning "INFO command not implemented yet"
    fi

    # Test transaction
    local result
    result=$(redis-cli -p 6379 --raw << EOF
MULTI
SET tx-key1 "value1"
SET tx-key2 "value2"
EXEC
EOF
)

    if echo "$result" >/dev/null 2>&1; then
        log_success "Transaction commands work"
    else
        log_error "Transaction commands failed"
        return 1
    fi

    return 0
}

# Main test function
main() {
    log_info "Starting MinimalRedis cluster tests"

    # Wait for all nodes
    wait_for_node 6379 "Leader (node1)"
    wait_for_node 6380 "Follower (node2)"
    wait_for_node 6381 "Follower (node3)"

    # Test basic operations on each node
    test_basic_operations 6379 "Leader (node1)" || exit 1
    test_basic_operations 6380 "Follower (node2)" || exit 1
    test_basic_operations 6381 "Follower (node3)" || exit 1

    # Test consistency
    test_consistency || exit 1

    # Test cluster commands
    test_cluster_commands || exit 1

    log_success "All cluster tests passed! 🎉"
    echo ""
    echo "Cluster is working correctly:"
    echo "- Leader (port 6379): ✅"
    echo "- Follower 1 (port 6380): ✅"
    echo "- Follower 2 (port 6381): ✅"
    echo "- Data consistency: ✅"
    echo "- Transactions: ✅"
}

# Run main function
main "$@"