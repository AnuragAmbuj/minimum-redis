#include "wal.h"
#include <filesystem>
#include <iostream>

WriteAheadLog::WriteAheadLog(const std::string& log_file)
    : log_file_(log_file), next_sequence_number_(1), max_log_size_(1024 * 1024 * 100) { // 100MB

    ensure_log_directory();

    // Open log file in append mode
    log_stream_.open(log_file_, std::ios::binary | std::ios::app);
    if (!log_stream_.is_open()) {
        throw std::runtime_error("Failed to open WAL file: " + log_file_);
    }
}

WriteAheadLog::~WriteAheadLog() {
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
}

bool WriteAheadLog::append_entry(const std::string& operation, const std::vector<std::string>& args) {
    std::lock_guard<std::mutex> lock(mutex_);

    WALEntry entry(next_sequence_number_++, operation, args);

    if (write_entry(entry)) {
        // Check if we need to rotate the log
        if (get_log_size() > max_log_size_) {
            rotate_log();
        }
        return true;
    }

    return false;
}

std::vector<WALEntry> WriteAheadLog::read_entries_from(uint64_t sequence_number) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<WALEntry> entries;

    std::ifstream stream(log_file_, std::ios::binary);
    if (!stream.is_open()) {
        return entries;
    }

    while (stream) {
        try {
            WALEntry entry = read_entry(stream);
            if (entry.sequence_number >= sequence_number) {
                entries.push_back(entry);
            }
        } catch (const std::exception&) {
            // End of valid entries
            break;
        }
    }

    return entries;
}

std::vector<WALEntry> WriteAheadLog::recover_log() {
    return read_entries_from(1);
}

void WriteAheadLog::rotate_log() {
    log_stream_.close();

    // Create backup of current log
    std::string backup_file = log_file_ + ".old";
    std::rename(log_file_.c_str(), backup_file.c_str());

    // Open new log file
    log_stream_.open(log_file_, std::ios::binary | std::ios::trunc);
    if (!log_stream_.is_open()) {
        throw std::runtime_error("Failed to rotate WAL file: " + log_file_);
    }

    // Reset sequence number for new log
    next_sequence_number_ = 1;

    // Optionally clean up old backup files
    // TODO: Implement cleanup policy
}

void WriteAheadLog::truncate_before(uint64_t sequence_number) {
    // For MVP, we'll implement a simple truncation by rewriting the log
    std::lock_guard<std::mutex> lock(mutex_);

    auto entries = read_entries_from(sequence_number);
    if (entries.empty()) return;

    // Close current stream
    log_stream_.close();

    // Rewrite log with remaining entries
    log_stream_.open(log_file_, std::ios::binary | std::ios::trunc);
    if (!log_stream_.is_open()) {
        throw std::runtime_error("Failed to truncate WAL file: " + log_file_);
    }

    for (const auto& entry : entries) {
        write_entry(entry);
    }
}

size_t WriteAheadLog::get_log_size() const {
    try {
        std::ifstream file(log_file_, std::ifstream::ate | std::ifstream::binary);
        return file.tellg();
    } catch (const std::exception&) {
        return 0;
    }
}

bool WriteAheadLog::write_entry(const WALEntry& entry) {
    if (!log_stream_.is_open()) return false;

    try {
        // Write sequence number
        log_stream_.write(reinterpret_cast<const char*>(&entry.sequence_number), sizeof(uint64_t));

        // Write timestamp
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()).count();
        log_stream_.write(reinterpret_cast<const char*>(&timestamp), sizeof(int64_t));

        // Write operation
        uint32_t op_len = entry.operation.length();
        log_stream_.write(reinterpret_cast<const char*>(&op_len), sizeof(uint32_t));
        log_stream_.write(entry.operation.data(), op_len);

        // Write args count
        uint32_t args_count = entry.args.size();
        log_stream_.write(reinterpret_cast<const char*>(&args_count), sizeof(uint32_t));

        // Write args
        for (const auto& arg : entry.args) {
            uint32_t arg_len = arg.length();
            log_stream_.write(reinterpret_cast<const char*>(&arg_len), sizeof(uint32_t));
            log_stream_.write(arg.data(), arg_len);
        }

        log_stream_.flush();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

WALEntry WriteAheadLog::read_entry(std::ifstream& stream) {
    WALEntry entry(0, "", {});

    // Read sequence number
    stream.read(reinterpret_cast<char*>(&entry.sequence_number), sizeof(uint64_t));
    if (!stream) throw std::runtime_error("Failed to read sequence number");

    // Read timestamp
    int64_t timestamp_ms;
    stream.read(reinterpret_cast<char*>(&timestamp_ms), sizeof(int64_t));
    if (!stream) throw std::runtime_error("Failed to read timestamp");
    entry.timestamp = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(timestamp_ms));

    // Read operation
    uint32_t op_len;
    stream.read(reinterpret_cast<char*>(&op_len), sizeof(uint32_t));
    if (!stream) throw std::runtime_error("Failed to read operation length");

    entry.operation.resize(op_len);
    stream.read(&entry.operation[0], op_len);
    if (!stream) throw std::runtime_error("Failed to read operation");

    // Read args count
    uint32_t args_count;
    stream.read(reinterpret_cast<char*>(&args_count), sizeof(uint32_t));
    if (!stream) throw std::runtime_error("Failed to read args count");

    // Read args
    for (uint32_t i = 0; i < args_count; ++i) {
        uint32_t arg_len;
        stream.read(reinterpret_cast<char*>(&arg_len), sizeof(uint32_t));
        if (!stream) throw std::runtime_error("Failed to read arg length");

        std::string arg;
        arg.resize(arg_len);
        stream.read(&arg[0], arg_len);
        if (!stream) throw std::runtime_error("Failed to read arg");

        entry.args.push_back(arg);
    }

    return entry;
}

void WriteAheadLog::ensure_log_directory() {
    // For MVP, we'll just ensure the file can be created
    // Directory creation is handled by the filesystem when opening files
}