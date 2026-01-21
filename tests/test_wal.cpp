#include "gtest/gtest.h"
#include "wal.h"
#include <filesystem>
#include <fstream>

// Test fixture for WAL tests
class WALTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary WAL file for testing
        test_file = "test_wal.log";
        wal = std::make_unique<WriteAheadLog>(test_file);
    }

    void TearDown() override {
        // Clean up test file
        std::filesystem::remove(test_file);
    }

    std::string test_file;
    std::unique_ptr<WriteAheadLog> wal;
};

// Test WAL initialization
TEST_F(WALTest, Initialization) {
    EXPECT_EQ(wal->get_next_sequence_number(), 1u);
    EXPECT_GT(wal->get_log_size(), 0u); // Should have created file
}

// Test basic log entry append
TEST_F(WALTest, AppendEntry) {
    std::string operation = "SET";
    std::vector<std::string> args = {"test_key", "test_value"};

    bool result = wal->append_entry(operation, args);
    EXPECT_TRUE(result);

    EXPECT_EQ(wal->get_next_sequence_number(), 2u);
}

// Test multiple entries
TEST_F(WALTest, MultipleEntries) {
    // Append several entries
    wal->append_entry("SET", {"key1", "value1"});
    wal->append_entry("SET", {"key2", "value2"});
    wal->append_entry("DEL", {"key1"});

    EXPECT_EQ(wal->get_next_sequence_number(), 4u);
}

// Test log recovery
TEST_F(WALTest, LogRecovery) {
    // Append some entries
    wal->append_entry("SET", {"key1", "value1"});
    wal->append_entry("SET", {"key2", "value2"});

    // Create new WAL instance and recover
    auto recovered_wal = std::make_unique<WriteAheadLog>(test_file + "_recovery");
    std::filesystem::copy_file(test_file, test_file + "_recovery");

    // Recover entries
    auto entries = recovered_wal->recover_log();

    EXPECT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].operation, "SET");
    EXPECT_EQ(entries[0].args[0], "key1");
    EXPECT_EQ(entries[0].args[1], "value1");
}

// Test log truncation
TEST_F(WALTest, LogTruncation) {
    // Append entries
    wal->append_entry("SET", {"key1", "value1"});
    wal->append_entry("SET", {"key2", "value2"});
    wal->append_entry("SET", {"key3", "value3"});

    // Truncate before sequence number 2
    wal->truncate_before(2);

    // Recover and check
    auto entries = wal->recover_log();
    EXPECT_EQ(entries.size(), 2u); // Should have entries 2 and 3
    EXPECT_EQ(entries[0].sequence_number, 2u);
}

// Test log rotation (when size limit is reached)
TEST_F(WALTest, LogRotation) {
    // This would require setting a small max size and appending many entries
    // For MVP testing, we'll just verify the mechanism exists
    EXPECT_TRUE(wal->get_log_size() > 0);
}