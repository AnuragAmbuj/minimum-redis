#pragma once

#include "interfaces.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
class OptimizedDataStore : public IDataStore {
private:
    struct KeyEntry {
        RedisValue value;
        uint64_t mod_count{0};
        std::chrono::steady_clock::time_point expiry_time;

        KeyEntry() = default;
        KeyEntry(RedisValue v) : value(std::move(v)) {}
    };

    std::unordered_map<std::string, KeyEntry> data_;
    mutable std::mutex data_mutex_;

    bool is_expired(const KeyEntry& entry) const noexcept;
    void cleanup_expired_keys();
    uint64_t generate_mod_count() noexcept;

public:
    OptimizedDataStore();

    bool set(const std::string& key, const RedisValue& value) override;
    std::unique_ptr<RedisValue> get(const std::string& key) override;
    bool del(const std::string& key) override;
    bool exists(const std::string& key) override;
    std::vector<std::string> keys(const std::string& pattern = "*") override;
    size_t dbsize() const override;
    bool expire(const std::string& key, std::chrono::seconds duration) override;
    bool pexpire(const std::string& key, std::chrono::milliseconds duration) override;
    std::chrono::seconds ttl(const std::string& key) override;
    std::chrono::milliseconds pttl(const std::string& key) override;
    bool persist(const std::string& key) override;
    uint64_t get_mod_count(const std::string& key) override;
    void increment_mod_count(const std::string& key) override;
    RedisType type(const std::string& key) override;
};

class OptimizedStringHandler : public IStringHandler {
private:
    OptimizedDataStore& store_;

public:
    explicit OptimizedStringHandler(OptimizedDataStore& store) : store_(store) {}

    std::string get(const std::string& key) override;
    bool set(const std::string& key, const std::string& value) override;
    size_t strlen(const std::string& key) override;
    std::string getrange(const std::string& key, int start, int end) override;
    size_t setrange(const std::string& key, size_t offset, const std::string& value) override;
};

// Memory-efficient PubSub with optimized subscriber management
class OptimizedPubSubManager : public IPubSubManager {
private:
    std::unordered_map<std::string, std::vector<int>> subscriptions_;
    mutable std::mutex subscriptions_mutex_;

public:
    void subscribe(int client_fd, const std::string& channel) override;
    void unsubscribe(int client_fd, const std::string& channel) override;
    void unsubscribe_all(int client_fd) override;
    std::vector<int> get_subscribers(const std::string& channel) override;
    void publish(const std::string& channel, const std::string& message) override;
};

// Optimized watch manager with efficient key tracking
class OptimizedWatchManager : public IWatchManager {
private:
    std::unordered_map<std::string, std::unordered_map<int, uint64_t>> watched_keys_;
    std::unordered_map<int, std::vector<std::string>> client_watches_;
    mutable std::mutex watch_mutex_;

public:
    bool watch_key(const std::string& key, uint64_t expected_version) override;
    bool unwatch_key(const std::string& key) override;
    bool unwatch_all() override;
    bool check_watched_keys() override;
    void clear_watch_state() override;
};