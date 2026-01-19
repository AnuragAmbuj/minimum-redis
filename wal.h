#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <chrono>

// Write-Ahead Log entry
struct WALEntry {
    uint64_t sequence_number;
    std::chrono::system_clock::time_point timestamp;
    std::string operation;
    std::vector<std::string> args;

    WALEntry(uint64_t seq, const std::string& op, const std::vector<std::string>& a)
        : sequence_number(seq), timestamp(std::chrono::system_clock::now()),
          operation(op), args(a) {}
};

// Write-Ahead Log for durable operation logging
class WriteAheadLog {
private:
    std::string log_file_;
    std::ofstream log_stream_;
    std::mutex mutex_;
    uint64_t next_sequence_number_;
    size_t max_log_size_; // Rotate log when it gets too large

public:
    WriteAheadLog(const std::string& log_file);
    ~WriteAheadLog();

    // Log operations
    bool append_entry(const std::string& operation, const std::vector<std::string>& args);
    std::vector<WALEntry> read_entries_from(uint64_t sequence_number);

    // Recovery
    std::vector<WALEntry> recover_log();

    // Maintenance
    void rotate_log();
    void truncate_before(uint64_t sequence_number);

    // Stats
    uint64_t get_next_sequence_number() const { return next_sequence_number_; }
    size_t get_log_size() const;

private:
    bool write_entry(const WALEntry& entry);
    WALEntry read_entry(std::ifstream& stream);
    void ensure_log_directory();
};