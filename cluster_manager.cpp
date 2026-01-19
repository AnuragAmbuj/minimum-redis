#include "cluster_manager.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

ClusterManager::ClusterManager(const std::string& node_id, const std::string& host, int port,
                               const std::vector<ClusterNode>& nodes, Database& db)
    : node_id_(node_id), host_(host), port_(port), nodes_(nodes), database_(db),
      server_socket_(-1), running_(false) {
}

ClusterManager::~ClusterManager() {
    stop();
}

void ClusterManager::start() {
    if (running_) return;

    running_ = true;

    // Initialize WAL
    std::string wal_file = "wal/" + node_id_ + ".log";
    wal_ = std::make_unique<WriteAheadLog>(wal_file);

    // Recover from WAL if needed
    auto recovered_entries = wal_->recover_log();
    for (const auto& entry : recovered_entries) {
        // Apply recovered operations to database
        // In a full implementation, this would replay operations
        std::cout << "[CLUSTER] Recovered operation: " << entry.operation << std::endl;
    }

    // Extract peer node IDs
    std::vector<std::string> peers;
    for (const auto& node : nodes_) {
        if (node.node_id != node_id_) {
            peers.push_back(node.node_id);
        }
    }

    // Initialize RAFT
    raft_ = std::make_unique<RaftConsensus>(node_id_, peers);
    raft_->set_callbacks(
        [this](const LogEntry& entry) { this->on_log_entry_committed(entry); },
        [this](const std::string& target, const std::string& msg) { this->send_raft_message(target, msg); }
    );

    // Start network server
    start_network_server();

    // Start background threads
    raft_thread_ = std::thread(&ClusterManager::raft_event_loop, this);
    network_thread_ = std::thread(&ClusterManager::network_event_loop, this);

    std::cout << "[CLUSTER] Cluster manager started for node " << node_id_ << std::endl;
}

void ClusterManager::stop() {
    if (!running_) return;

    running_ = false;

    // Close network connections
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }

    for (const auto& pair : client_sockets_) {
        close(pair.first);
    }
    client_sockets_.clear();

    // Stop threads
    if (raft_thread_.joinable()) {
        raft_thread_.join();
    }
    if (network_thread_.joinable()) {
        network_thread_.join();
    }

    std::cout << "[CLUSTER] Cluster manager stopped" << std::endl;
}

bool ClusterManager::process_command(const std::string& command, const std::vector<std::string>& args) {
    if (!running_ || !raft_->is_leader()) {
        // Forward to leader if we're not the leader
        // For MVP, we'll just reject non-leader operations
        return false;
    }

    // Log the operation to WAL
    if (!wal_->append_entry(command, args)) {
        std::cerr << "[CLUSTER] Failed to log operation to WAL" << std::endl;
        return false;
    }

    // Append to RAFT log
    std::stringstream operation_stream;
    operation_stream << command;
    for (const auto& arg : args) {
        operation_stream << " " << arg;
    }

    if (!raft_->append_log_entry(operation_stream.str())) {
        std::cerr << "[CLUSTER] Failed to append to RAFT log" << std::endl;
        return false;
    }

    return true;
}

void ClusterManager::replicate_operation(const std::string& operation, const std::vector<std::string>& args) {
    // Apply operation to local database
    // This would parse and execute the operation
    std::cout << "[CLUSTER] Applying replicated operation: " << operation << std::endl;
}

void ClusterManager::on_log_entry_committed(const LogEntry& entry) {
    // Parse and apply the committed log entry
    std::stringstream ss(entry.command);
    std::string command;
    ss >> command;

    std::vector<std::string> args;
    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }

    replicate_operation(command, args);
}

void ClusterManager::send_raft_message(const std::string& target_node, const std::string& message) {
    send_to_node(target_node, "RAFT:" + message);
}

void ClusterManager::start_network_server() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        throw std::runtime_error("Failed to create server socket");
    }

    // Set socket options
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host_.c_str());
    addr.sin_port = htons(port_);

    if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(server_socket_, 10) < 0) {
        throw std::runtime_error("Failed to listen on server socket");
    }

    // Set non-blocking
    fcntl(server_socket_, F_SETFL, O_NONBLOCK);

    std::cout << "[CLUSTER] Listening on " << host_ << ":" << port_ << std::endl;
}

void ClusterManager::network_event_loop() {
    while (running_) {
        // Use poll for network events
        std::vector<pollfd> fds;

        // Add server socket
        pollfd server_pollfd = {server_socket_, POLLIN, 0};
        fds.push_back(server_pollfd);

        // Add client sockets
        {
            std::lock_guard<std::mutex> lock(network_mutex_);
            for (const auto& pair : client_sockets_) {
                pollfd client_pollfd = {pair.first, POLLIN, 0};
                fds.push_back(client_pollfd);
            }
        }

        int timeout_ms = 100; // 100ms timeout
        int result = poll(fds.data(), fds.size(), timeout_ms);

        if (result > 0) {
            // Handle server socket
            if (fds[0].revents & POLLIN) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);

                if (client_fd >= 0) {
                    handle_incoming_connection(client_fd);
                }
            }

            // Handle client sockets
            for (size_t i = 1; i < fds.size(); ++i) {
                if (fds[i].revents & POLLIN) {
                    char buffer[1024];
                    ssize_t n = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (n > 0) {
                        buffer[n] = '\0';
                        handle_message(fds[i].fd, std::string(buffer));
                    } else {
                        // Connection closed or error
                        close(fds[i].fd);
                        std::lock_guard<std::mutex> lock(network_mutex_);
                        client_sockets_.erase(fds[i].fd);
                    }
                }
            }
        }
    }
}

void ClusterManager::handle_incoming_connection(int client_fd) {
    // Set non-blocking
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // For MVP, we'll accept any connection
    // In a real implementation, we'd validate the connecting node
    std::lock_guard<std::mutex> lock(network_mutex_);
    client_sockets_[client_fd] = "unknown"; // Would be determined by handshake

    std::cout << "[CLUSTER] New connection from client " << client_fd << std::endl;
}

void ClusterManager::handle_message(int client_fd, const std::string& message) {
    if (message.find("RAFT:") == 0) {
        // Handle RAFT messages
        std::string raft_message = message.substr(5);

        // Parse RAFT message and call appropriate raft methods
        if (raft_message.find("REQUEST_VOTE") == 0) {
            // Parse and handle request vote
            std::cout << "[CLUSTER] Received REQUEST_VOTE from " << client_fd << std::endl;
        } else if (raft_message.find("APPEND_ENTRIES") == 0) {
            // Parse and handle append entries
            std::cout << "[CLUSTER] Received APPEND_ENTRIES from " << client_fd << std::endl;
        }
        // Add more RAFT message handling as needed
    }
}

void ClusterManager::send_to_node(const std::string& node_id, const std::string& message) {
    // Find the socket for this node
    std::lock_guard<std::mutex> lock(network_mutex_);
    for (const auto& pair : client_sockets_) {
        if (pair.second == node_id) {
            send(pair.first, message.c_str(), message.length(), 0);
            return;
        }
    }

    // If not connected, we'd need to establish connection
    // For MVP, we'll just log
    std::cout << "[CLUSTER] Would send to " << node_id << ": " << message << std::endl;
}

void ClusterManager::raft_event_loop() {
    while (running_) {
        // Send heartbeats if leader
        raft_->send_heartbeats();

        // Check election timeout
        raft_->check_election_timeout();

        // Sleep for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

uint64_t ClusterManager::get_key_version(const std::string& key) {
    std::lock_guard<std::mutex> lock(version_mutex_);
    return key_versions_[key];
}

void ClusterManager::update_key_version(const std::string& key, uint64_t version) {
    std::lock_guard<std::mutex> lock(version_mutex_);
    key_versions_[key] = version;
}

void ClusterManager::resolve_conflicts(const std::string& key, const std::vector<std::string>& conflicting_values) {
    // Simple conflict resolution: last write wins
    // In a real implementation, this would be more sophisticated
    std::cout << "[CLUSTER] Resolving conflict for key " << key << std::endl;
}