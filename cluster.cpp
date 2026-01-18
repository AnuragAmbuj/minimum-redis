#include "cluster.h"
#include <algorithm>
#include <chrono>

// Global cluster configuration instance
ClusterConfig cluster_config;

std::shared_ptr<ClusterNode> ClusterConfig::get_node_for_key(const std::string& key) {
    if (!enabled || nodes.empty()) {
        return nullptr;
    }

    // Simple hash function for MVC - use std::hash for basic distribution
    size_t hash = std::hash<std::string>{}(key);
    int slot = hash % total_slots;

    // Find node responsible for this slot (simple round-robin for MVC)
    for (auto& node : nodes) {
        if (std::find(node->assigned_slots.begin(), node->assigned_slots.end(), slot) != node->assigned_slots.end()) {
            return node;
        }
    }

    // Fallback to first node if no assignment found
    return nodes[0];
}

std::shared_ptr<ClusterNode> ClusterConfig::get_node_by_id(const std::string& node_id) {
    for (auto& node : nodes) {
        if (node->node_id == node_id) {
            return node;
        }
    }
    return nullptr;
}

// Initialize cluster for MVC demo
void initialize_cluster_mvc() {
    cluster_config.enabled = true;
    cluster_config.my_node_id = "node1";
    cluster_config.my_ip = "127.0.0.1";
    cluster_config.my_port = 6379;
    cluster_config.total_slots = 4;

    // Create demo nodes
    auto node1 = std::make_shared<ClusterNode>("node1", "127.0.0.1", 6379, true);
    node1->assigned_slots = {0, 1}; // Slots 0-1

    auto node2 = std::make_shared<ClusterNode>("node2", "127.0.0.1", 6380, false);
    node2->assigned_slots = {2, 3}; // Slots 2-3

    cluster_config.nodes.push_back(node1);
    cluster_config.nodes.push_back(node2);
}