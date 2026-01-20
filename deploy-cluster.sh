#!/bin/bash

# Multi-Cluster Redis Deployment Script
# This script deploys a 3-node Redis cluster using Docker

set -e

# Configuration
CLUSTER_NAME="minimal-redis-cluster"
NUM_NODES=3
BASE_PORT=6379
DOCKER_IMAGE="minimal-redis:latest"
NETWORK_NAME="${CLUSTER_NAME}-network"

# Colors for output
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

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Docker is installed and running
check_docker() {
    log_info "Checking Docker installation..."

    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed. Please install Docker first."
        exit 1
    fi

    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running. Please start Docker."
        exit 1
    fi

    log_success "Docker is available"
}

# Build the MinimalRedis Docker image
build_image() {
    log_info "Building MinimalRedis Docker image..."

    # Check if Dockerfile exists
    if [ ! -f "Dockerfile" ]; then
        log_error "Dockerfile not found in current directory"
        exit 1
    fi

    # Build the image
    docker build -t "$DOCKER_IMAGE" .

    if [ $? -eq 0 ]; then
        log_success "Docker image built successfully"
    else
        log_error "Failed to build Docker image"
        exit 1
    fi
}

# Create Docker network
create_network() {
    log_info "Creating Docker network: $NETWORK_NAME"

    # Remove network if it already exists
    docker network rm "$NETWORK_NAME" &> /dev/null || true

    # Create new network
    docker network create "$NETWORK_NAME"

    log_success "Network created"
}

# Start a cluster node
start_node() {
    local node_id=$1
    local port=$2
    local is_leader=$3
    local container_name="${CLUSTER_NAME}-node${node_id}"

    log_info "Starting node $node_id on port $port (leader: $is_leader)"

    # Stop and remove existing container if it exists
    docker rm -f "$container_name" &> /dev/null || true

    # Environment variables for the node
    local env_vars=""
    if [ "$is_leader" = true ]; then
        env_vars="-e REDIS_LEADER=true"
    fi

    # Start the container
    docker run -d \
        --name "$container_name" \
        --network "$NETWORK_NAME" \
        -p "$port:$port" \
        $env_vars \
        -e NODE_ID="node$node_id" \
        -e CLUSTER_PORT="$port" \
        -e PEER_NODES="node1:$((BASE_PORT)),node2:$((BASE_PORT+1)),node3:$((BASE_PORT+2))" \
        "$DOCKER_IMAGE" \
        --port "$port" \
        --cluster-enabled \
        --node-id "node$node_id"

    log_success "Node $node_id started"
}

# Wait for a node to be ready
wait_for_node() {
    local port=$1
    local max_attempts=30
    local attempt=1

    log_info "Waiting for node on port $port to be ready..."

    while [ $attempt -le $max_attempts ]; do
        if nc -z localhost "$port" 2>/dev/null; then
            log_success "Node on port $port is ready"
            return 0
        fi

        echo -n "."
        sleep 2
        ((attempt++))
    done

    log_error "Node on port $port failed to start within timeout"
    return 1
}

# Configure cluster topology
configure_cluster() {
    log_info "Configuring cluster topology..."

    # Wait for all nodes to be ready
    for i in $(seq 1 $NUM_NODES); do
        local port=$((BASE_PORT + i - 1))
        wait_for_node "$port"
    done

    # For MVP, we'll use a simple configuration
    # In a full implementation, we'd use Redis cluster management commands
    log_success "Cluster topology configured"
}

# Start the entire cluster
start_cluster() {
    log_info "Starting $NUM_NODES-node Redis cluster..."

    # Start nodes (node1 is the leader)
    start_node 1 $BASE_PORT true   # Leader
    start_node 2 $((BASE_PORT+1)) false  # Follower
    start_node 3 $((BASE_PORT+2)) false  # Follower

    # Configure cluster
    configure_cluster

    log_success "Cluster started successfully!"
    echo ""
    echo "Cluster Information:"
    echo "==================="
    echo "Leader Node:  localhost:$BASE_PORT (node1)"
    echo "Follower 1:   localhost:$((BASE_PORT+1)) (node2)"
    echo "Follower 2:   localhost:$((BASE_PORT+2)) (node3)"
    echo ""
    echo "Network:      $NETWORK_NAME"
    echo ""
    echo "To connect to the cluster:"
    echo "  redis-cli -p $BASE_PORT"
    echo ""
    echo "To stop the cluster:"
    echo "  $0 stop"
}

# Stop the cluster
stop_cluster() {
    log_info "Stopping Redis cluster..."

    # Stop all containers
    for i in $(seq 1 $NUM_NODES); do
        local container_name="${CLUSTER_NAME}-node${i}"
        log_info "Stopping $container_name..."
        docker stop "$container_name" &> /dev/null || true
        docker rm "$container_name" &> /dev/null || true
    done

    # Remove network
    log_info "Removing network $NETWORK_NAME..."
    docker network rm "$NETWORK_NAME" &> /dev/null || true

    log_success "Cluster stopped"
}

# Show cluster status
show_status() {
    log_info "Cluster Status:"
    echo ""

    echo "Containers:"
    docker ps --filter "name=${CLUSTER_NAME}" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

    echo ""
    echo "Networks:"
    docker network ls --filter "name=${NETWORK_NAME}" --format "table {{.Name}}\t{{.Driver}}"

    echo ""
    echo "Node Health:"
    for i in $(seq 1 $NUM_NODES); do
        local port=$((BASE_PORT + i - 1))
        local container_name="${CLUSTER_NAME}-node${i}"

        if docker ps --format "{{.Names}}" | grep -q "^${container_name}$"; then
            if nc -z localhost "$port" 2>/dev/null; then
                echo -e "  node$i (port $port): ${GREEN}HEALTHY${NC}"
            else
                echo -e "  node$i (port $port): ${RED}UNHEALTHY${NC}"
            fi
        else
            echo -e "  node$i (port $port): ${RED}STOPPED${NC}"
        fi
    done
}

# Show usage
show_usage() {
    echo "Multi-Cluster Redis Deployment Script"
    echo ""
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  start     Start the Redis cluster"
    echo "  stop      Stop the Redis cluster"
    echo "  status    Show cluster status"
    echo "  restart   Restart the cluster"
    echo "  logs      Show logs from all nodes"
    echo "  help      Show this help message"
    echo ""
    echo "Environment Variables:"
    echo "  NUM_NODES    Number of cluster nodes (default: 3)"
    echo "  BASE_PORT    Starting port number (default: 6379)"
    echo "  CLUSTER_NAME Cluster name prefix (default: minimal-redis-cluster)"
}

# Show logs from all nodes
show_logs() {
    log_info "Cluster Logs:"
    echo ""

    for i in $(seq 1 $NUM_NODES); do
        local container_name="${CLUSTER_NAME}-node${i}"
        echo "=== Logs for $container_name ==="
        docker logs "$container_name" 2>&1 | tail -20
        echo ""
    done
}

# Restart cluster
restart_cluster() {
    log_info "Restarting cluster..."
    stop_cluster
    sleep 2
    start_cluster
}

# Main script logic
main() {
    local command=${1:-"help"}

    case "$command" in
        start)
            check_docker
            build_image
            create_network
            start_cluster
            ;;
        stop)
            stop_cluster
            ;;
        status)
            show_status
            ;;
        restart)
            restart_cluster
            ;;
        logs)
            show_logs
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            log_error "Unknown command: $command"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"