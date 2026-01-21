#include "gtest/gtest.h"
#include "raft.h"
#include <thread>
#include <chrono>

// Test fixture for RAFT tests
class RaftTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a RAFT instance for testing
        raft = std::make_unique<RaftConsensus>("test-node", std::vector<std::string>{"peer1", "peer2"});

        // Mock callbacks
        raft->set_callbacks(
            [](const LogEntry& entry) {
                // Mock log application
            },
            [](const std::string& target, const std::string& msg) {
                // Mock message sending
            }
        );
    }

    std::unique_ptr<RaftConsensus> raft;
};

// Basic RAFT initialization test
TEST_F(RaftTest, Initialization) {
    EXPECT_FALSE(raft->is_leader());
    EXPECT_EQ(raft->get_current_term(), 0u);
    EXPECT_EQ(raft->get_log_size(), 1u); // Dummy entry
}

// Test leader election basics
TEST_F(RaftTest, LeaderElection) {
    // Initially should be follower
    EXPECT_EQ(raft->get_state(), RaftState::FOLLOWER);

    // Start election
    raft->start_election();

    // Should become candidate
    EXPECT_EQ(raft->get_state(), RaftState::CANDIDATE);
}

// Test log entry appending
TEST_F(RaftTest, LogEntryAppend) {
    std::string test_command = "SET test_key test_value";

    bool result = raft->append_log_entry(test_command);
    EXPECT_TRUE(result); // Should succeed as leader (initially)

    EXPECT_EQ(raft->get_log_size(), 2u); // Dummy + new entry
}

// Test log replication
TEST_F(RaftTest, LogReplication) {
    // Make it a leader
    raft->become_leader();

    // Append some entries
    raft->append_log_entry("SET key1 value1");
    raft->append_log_entry("SET key2 value2");

    EXPECT_EQ(raft->get_log_size(), 3u); // Dummy + 2 entries
}

// Test term management
TEST_F(RaftTest, TermManagement) {
    uint64_t initial_term = raft->get_current_term();

    // Handle append entries from higher term
    raft->handle_append_entries(5, "leader", 0, 0, {}, 0);

    EXPECT_EQ(raft->get_current_term(), 5u);
    EXPECT_EQ(raft->get_state(), RaftState::FOLLOWER);
}

// Test heartbeat mechanism
TEST_F(RaftTest, HeartbeatMechanism) {
    // Initially follower
    EXPECT_EQ(raft->get_state(), RaftState::FOLLOWER);

    // Send heartbeat
    raft->send_heartbeats();

    // Should remain follower (no peers to send to)
    EXPECT_EQ(raft->get_state(), RaftState::FOLLOWER);
}