#pragma once

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "raft.h"
#include "wal.h"
#include "db.h"

// Cluster node information
struct ClusterNode {
    std::string node_id;
    std::string host;
    int port;
    bool is_alive;
    uint64_t last_seen;
};

// Cluster manager implementing log-based replication with RAFT consensus
class ClusterManager {
private:
    std::string node_id_;
    std::string host_;
    int port_;
    std::vector<ClusterNode> nodes_;

    // Core components
    std::unique_ptr<RaftConsensus> raft_;
    std::unique_ptr<WriteAheadLog> wal_;
    Database& database_;

    // Networking
    int server_socket_;
    std::unordered_map<int, std::string> client_sockets_; // fd -> node_id
    std::mutex network_mutex_;

    // Background threads
    std::thread raft_thread_;
    std::thread network_thread_;
    std::atomic<bool> running_;

    // Eventual consistency
    std::unordered_map<std::string, uint64_t> key_versions_;
    std::mutex version_mutex_;

public:
    ClusterManager(const std::string& node_id, const std::string& host, int port,
                   const std::vector<ClusterNode>& nodes, Database& db);
    ~ClusterManager();

    // Lifecycle
    void start();
    void stop();

    // Client operations
    bool process_command(const std::string& command, const std::vector<std::string>& args);

    // Replication
    void replicate_operation(const std::string& operation, const std::vector<std::string>& args);

    // Networking
    void handle_incoming_connection(int client_fd);
    void handle_message(int client_fd, const std::string& message);

    // Eventual consistency
    uint64_t get_key_version(const std::string& key);
    void update_key_version(const std::string& key, uint64_t version);

private:
    // RAFT callbacks
    void on_log_entry_committed(const LogEntry& entry);
    void send_raft_message(const std::string& target_node, const std::string& message);

    // Network operations
    void start_network_server();
    void network_event_loop();
    void send_to_node(const std::string& node_id, const std::string& message);

    // RAFT operations
    void raft_event_loop();

    // Conflict resolution for eventual consistency
    void resolve_conflicts(const std::string& key, const std::vector<std::string>& conflicting_values);
};