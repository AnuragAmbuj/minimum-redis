#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Cluster node information
struct ClusterNode {
    std::string node_id;
    std::string ip_address;
    int port;
    bool is_master;
    uint64_t last_seen;
    std::vector<int> assigned_slots; // For MVC, simple slot assignment

    ClusterNode(const std::string& id, const std::string& ip, int p, bool master = false)
        : node_id(id), ip_address(ip), port(p), is_master(master), last_seen(0) {}
};

// Cluster configuration
struct ClusterConfig {
    bool enabled = false;
    std::string my_node_id;
    std::string my_ip = "127.0.0.1";
    int my_port = 6379;
    std::vector<std::shared_ptr<ClusterNode>> nodes;
    int total_slots = 4; // Small number for MVC demo

    // Get node responsible for a key
    std::shared_ptr<ClusterNode> get_node_for_key(const std::string& key);
    // Get node by ID
    std::shared_ptr<ClusterNode> get_node_by_id(const std::string& node_id);
};

// Global cluster configuration
extern ClusterConfig cluster_config;

// Initialize cluster for MVC demo
void initialize_cluster_mvc();