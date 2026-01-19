#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <atomic>

// RAFT consensus states
enum class RaftState {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

// Log entry for replication
struct LogEntry {
    uint64_t term;
    uint64_t index;
    std::string command;
    std::chrono::system_clock::time_point timestamp;

    LogEntry() : term(0), index(0), command(""), timestamp(std::chrono::system_clock::now()) {}
    LogEntry(uint64_t t, uint64_t i, const std::string& cmd)
        : term(t), index(i), command(cmd), timestamp(std::chrono::system_clock::now()) {}
};

// RAFT consensus algorithm implementation
class RaftConsensus {
private:
    // Persistent state on all servers
    uint64_t current_term_;
    std::string voted_for_;
    std::vector<LogEntry> log_;

    // Volatile state on all servers
    uint64_t commit_index_;
    uint64_t last_applied_;

    // Volatile state on leaders
    std::unordered_map<std::string, uint64_t> next_index_;
    std::unordered_map<std::string, uint64_t> match_index_;

    // Server state
    RaftState state_;
    std::string server_id_;
    std::vector<std::string> peer_ids_;
    std::mutex mutex_;

    // Election timeout
    std::chrono::milliseconds election_timeout_;
    std::chrono::system_clock::time_point last_heartbeat_;

    // Callbacks for external integration
    std::function<void(const LogEntry&)> apply_log_entry_;
    std::function<void(const std::string&, const std::string&)> send_message_;

public:
    RaftConsensus(const std::string& server_id, const std::vector<std::string>& peers);

    // Core RAFT operations
    void start_election();
    void become_follower(uint64_t term);
    void become_candidate();
    void become_leader();

    // Message handling
    void handle_request_vote(const std::string& candidate_id, uint64_t term,
                           uint64_t last_log_index, uint64_t last_log_term);
    void handle_append_entries(uint64_t term, const std::string& leader_id,
                             uint64_t prev_log_index, uint64_t prev_log_term,
                             const std::vector<LogEntry>& entries, uint64_t leader_commit);
    void handle_vote_response(const std::string& voter_id, uint64_t term, bool granted);
    void handle_append_response(const std::string& peer_id, uint64_t term, bool success,
                              uint64_t match_index);

    // Client operations
    bool append_log_entry(const std::string& command);
    void apply_committed_entries();

    // Periodic operations
    void send_heartbeats();
    void check_election_timeout();

    // State queries
    RaftState get_state() const { return state_; }
    uint64_t get_current_term() const { return current_term_; }
    std::string get_leader_id() const;
    size_t get_log_size() const { return log_.size(); }
    bool is_leader() const { return state_ == RaftState::LEADER; }

    // Configuration
    void set_callbacks(std::function<void(const LogEntry&)> apply_fn,
                      std::function<void(const std::string&, const std::string&)> send_fn);

private:
    // Helper methods
    bool log_up_to_date(uint64_t last_log_index, uint64_t last_log_term) const;
    uint64_t get_last_log_index() const;
    uint64_t get_last_log_term() const;
    void persist_state();
    void restore_state();
};