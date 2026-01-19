#include "raft.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <sstream>

RaftConsensus::RaftConsensus(const std::string& server_id, const std::vector<std::string>& peers)
    : current_term_(0), voted_for_(""), commit_index_(0), last_applied_(0),
      state_(RaftState::FOLLOWER), server_id_(server_id), peer_ids_(peers),
      election_timeout_(std::chrono::milliseconds(150 + rand() % 150)) {

    // Initialize log with a dummy entry
    log_.emplace_back(0, 0, "");

    // Initialize leader state
    for (const auto& peer : peers) {
        next_index_[peer] = 1;
        match_index_[peer] = 0;
    }

    last_heartbeat_ = std::chrono::system_clock::now();
}

void RaftConsensus::set_callbacks(std::function<void(const LogEntry&)> apply_fn,
                                 std::function<void(const std::string&, const std::string&)> send_fn) {
    apply_log_entry_ = apply_fn;
    send_message_ = send_fn;
}

void RaftConsensus::start_election() {
    std::lock_guard<std::mutex> lock(mutex_);
    become_candidate();
}

void RaftConsensus::become_follower(uint64_t term) {
    state_ = RaftState::FOLLOWER;
    current_term_ = term;
    voted_for_ = "";
    last_heartbeat_ = std::chrono::system_clock::now();
}

void RaftConsensus::become_candidate() {
    state_ = RaftState::CANDIDATE;
    current_term_++;
    voted_for_ = server_id_;

    // Reset election timeout
    election_timeout_ = std::chrono::milliseconds(150 + rand() % 150);
    last_heartbeat_ = std::chrono::system_clock::now();

    // Request votes from all peers
    for (const auto& peer : peer_ids_) {
        if (send_message_) {
            std::stringstream msg;
            msg << "REQUEST_VOTE " << current_term_ << " "
                << get_last_log_index() << " " << get_last_log_term();
            send_message_(peer, msg.str());
        }
    }
}

void RaftConsensus::become_leader() {
    state_ = RaftState::LEADER;

    // Initialize next_index and match_index
    for (const auto& peer : peer_ids_) {
        next_index_[peer] = log_.size();
        match_index_[peer] = 0;
    }

    // Send initial heartbeats
    send_heartbeats();
}

void RaftConsensus::handle_request_vote(const std::string& candidate_id, uint64_t term,
                                       uint64_t last_log_index, uint64_t last_log_term) {
    std::lock_guard<std::mutex> lock(mutex_);

    bool grant_vote = false;

    if (term > current_term_) {
        become_follower(term);
    }

    if (term == current_term_ &&
        (voted_for_.empty() || voted_for_ == candidate_id) &&
        log_up_to_date(last_log_index, last_log_term)) {

        voted_for_ = candidate_id;
        grant_vote = true;
        last_heartbeat_ = std::chrono::system_clock::now();
    }

    // Send vote response
    if (send_message_) {
        std::stringstream msg;
        msg << "VOTE_RESPONSE " << current_term_ << " " << (grant_vote ? "1" : "0");
        send_message_(candidate_id, msg.str());
    }
}

void RaftConsensus::handle_append_entries(uint64_t term, const std::string& leader_id,
                                         uint64_t prev_log_index, uint64_t prev_log_term,
                                         const std::vector<LogEntry>& entries, uint64_t leader_commit) {
    std::lock_guard<std::mutex> lock(mutex_);

    bool success = false;

    if (term > current_term_) {
        become_follower(term);
    }

    if (term == current_term_) {
        state_ = RaftState::FOLLOWER;
        last_heartbeat_ = std::chrono::system_clock::now();

        // Check if log is consistent
        if (prev_log_index == 0 ||
            (prev_log_index < log_.size() && log_[prev_log_index].term == prev_log_term)) {

            success = true;

            // Append new entries
            for (size_t i = 0; i < entries.size(); ++i) {
                uint64_t entry_index = prev_log_index + 1 + i;
                if (entry_index < log_.size()) {
                    if (log_[entry_index].term != entries[i].term) {
                        // Truncate inconsistent entries
                        log_.resize(entry_index);
                    }
                }
                if (entry_index >= log_.size()) {
                    log_.push_back(entries[i]);
                }
            }

            // Update commit index
            if (leader_commit > commit_index_) {
                commit_index_ = std::min(leader_commit, static_cast<uint64_t>(log_.size() - 1));
                apply_committed_entries();
            }
        }
    }

    // Send append response
    if (send_message_) {
        std::stringstream msg;
        msg << "APPEND_RESPONSE " << current_term_ << " " << (success ? "1" : "0") << " "
            << (log_.size() - 1);
        send_message_(leader_id, msg.str());
    }
}

void RaftConsensus::handle_vote_response(const std::string& voter_id, uint64_t term, bool granted) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (term > current_term_) {
        become_follower(term);
        return;
    }

    if (state_ == RaftState::CANDIDATE && term == current_term_ && granted) {
        // Count votes (simplified - just check if we have majority)
        int votes = 1; // Vote for self
        for (const auto& peer : peer_ids_) {
            // In a real implementation, we'd track individual votes
            votes++;
        }

        if (votes > static_cast<int>(peer_ids_.size() + 1) / 2) {
            become_leader();
        }
    }
}

void RaftConsensus::handle_append_response(const std::string& peer_id, uint64_t term, bool success,
                                          uint64_t match_index) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (term > current_term_) {
        become_follower(term);
        return;
    }

    if (state_ == RaftState::LEADER && term == current_term_) {
        if (success) {
            match_index_[peer_id] = match_index;
            next_index_[peer_id] = match_index + 1;

            // Update commit index
            uint64_t new_commit_index = commit_index_;
            for (uint64_t i = commit_index_ + 1; i < log_.size(); ++i) {
                int replicated_count = 0;
                for (const auto& match : match_index_) {
                    if (match.second >= i) replicated_count++;
                }
                if (replicated_count >= static_cast<int>(peer_ids_.size() + 1) / 2) {
                    new_commit_index = i;
                }
            }
            commit_index_ = new_commit_index;
            apply_committed_entries();
        } else {
            // Decrement next_index and retry
            next_index_[peer_id] = std::max(static_cast<uint64_t>(1), next_index_[peer_id] - 1);
        }
    }
}

bool RaftConsensus::append_log_entry(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (state_ != RaftState::LEADER) {
        return false;
    }

    LogEntry entry(current_term_, log_.size(), command);
    log_.push_back(entry);

    // Send append entries to all peers
    send_heartbeats();

    return true;
}

void RaftConsensus::apply_committed_entries() {
    while (last_applied_ < commit_index_) {
        last_applied_++;
        if (apply_log_entry_ && last_applied_ < log_.size()) {
            apply_log_entry_(log_[last_applied_]);
        }
    }
}

void RaftConsensus::send_heartbeats() {
    if (state_ != RaftState::LEADER) return;

    for (const auto& peer : peer_ids_) {
        uint64_t prev_log_index = next_index_[peer] - 1;
        uint64_t prev_log_term = (prev_log_index > 0 && prev_log_index < log_.size()) ?
                                log_[prev_log_index].term : 0;

        std::vector<LogEntry> entries;
        for (uint64_t i = next_index_[peer]; i < log_.size(); ++i) {
            entries.push_back(log_[i]);
        }

        if (send_message_) {
            std::stringstream msg;
            msg << "APPEND_ENTRIES " << current_term_ << " " << server_id_ << " "
                << prev_log_index << " " << prev_log_term << " "
                << entries.size() << " " << commit_index_;

            for (const auto& entry : entries) {
                msg << " " << entry.term << " " << entry.index << " " << entry.command.length()
                    << " " << entry.command;
            }

            send_message_(peer, msg.str());
        }
    }
}

void RaftConsensus::check_election_timeout() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat_);

    if (state_ != RaftState::LEADER && elapsed > election_timeout_) {
        start_election();
    }
}

std::string RaftConsensus::get_leader_id() const {
    // In this MVP, we don't track the leader ID
    // In a full implementation, we'd maintain this state
    return "";
}

bool RaftConsensus::log_up_to_date(uint64_t last_log_index, uint64_t last_log_term) const {
    uint64_t my_last_index = get_last_log_index();
    uint64_t my_last_term = get_last_log_term();

    return (last_log_term > my_last_term) ||
           (last_log_term == my_last_term && last_log_index >= my_last_index);
}

uint64_t RaftConsensus::get_last_log_index() const {
    return log_.size() - 1;
}

uint64_t RaftConsensus::get_last_log_term() const {
    return log_.empty() ? 0 : log_.back().term;
}

void RaftConsensus::persist_state() {
    // TODO: Implement state persistence
}

void RaftConsensus::restore_state() {
    // TODO: Implement state restoration
}