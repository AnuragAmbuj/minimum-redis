#!/bin/bash

# Test Cluster Deployment Script
# This script deploys a 3-node MinimalRedis cluster locally and launches TUI for testing

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_SCRIPT="$SCRIPT_DIR/deploy-cluster.sh"
TUI_CLIENT="$SCRIPT_DIR/clients/tui/build/redis-tui"

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

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check if deploy-cluster.sh exists
    if [ ! -f "$DEPLOY_SCRIPT" ]; then
        log_error "deploy-cluster.sh not found at $DEPLOY_SCRIPT"
        exit 1
    fi

    # Check if TUI client exists
    if [ ! -f "$TUI_CLIENT" ]; then
        log_error "TUI client not found at $TUI_CLIENT. Please build it first."
        exit 1
    fi

    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed. Please install Docker first."
        exit 1
    fi

    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running. Please start Docker."
        exit 1
    fi

    log_success "Prerequisites check passed"
}

# Deploy the cluster
deploy_cluster() {
    log_info "Deploying 3-node cluster..."

    if ! "$DEPLOY_SCRIPT" start; then
        log_error "Failed to deploy cluster"
        exit 1
    fi

    log_success "Cluster deployed successfully"
}

# Launch TUI for testing
launch_tui() {
    log_info "Launching Redis TUI for cluster testing..."
    log_info "Use option (4) Cluster Monitor to view cluster status"
    log_info "Press Ctrl+C to exit TUI and return to this script"

    # Launch TUI and wait for it to exit
    "$TUI_CLIENT" || true  # Don't fail if TUI exits with error

    log_info "TUI exited"
}

# Show cluster status
show_status() {
    log_info "Cluster Status:"
    "$DEPLOY_SCRIPT" status
}

# Stop the cluster
stop_cluster() {
    log_info "Stopping cluster..."
    "$DEPLOY_SCRIPT" stop
    log_success "Cluster stopped"
}

# Cleanup function
cleanup() {
    log_warning "Received interrupt signal. Cleaning up..."
    stop_cluster
    exit 1
}

# Trap interrupt signals
trap cleanup SIGINT SIGTERM

# Main script logic
main() {
    local command=${1:-"test"}

    case "$command" in
        start)
            check_prerequisites
            deploy_cluster
            ;;
        test)
            check_prerequisites
            deploy_cluster
            echo ""
            echo "========================================"
            echo "Cluster is running! Starting TUI test..."
            echo "========================================"
            echo ""
            launch_tui
            echo ""
            echo "========================================"
            echo "TUI test completed. Checking cluster status..."
            echo "========================================"
            show_status
            ;;
        stop)
            stop_cluster
            ;;
        status)
            show_status
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

# Show usage
show_usage() {
    echo "Test Cluster Deployment Script"
    echo ""
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  test     Deploy cluster and launch TUI for testing (default)"
    echo "  start    Deploy cluster only"
    echo "  stop     Stop the cluster"
    echo "  status   Show cluster status"
    echo "  help     Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 test      # Deploy cluster and test with TUI"
    echo "  $0 start     # Just start the cluster"
    echo "  $0 status    # Check cluster status"
    echo "  $0 stop      # Stop the cluster"
}

# Run main function with all arguments
main "$@"