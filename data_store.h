#pragma once

#include "interfaces.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <optional>
class MemoryPool {
private:
    static constexpr size_t BLOCK_SIZE = 4096;
    static constexpr size_t MAX_BLOCKS = 1024;

    std::vector<std::unique_ptr<char[]>> blocks_;
    std::vector<size_t> free_offsets_;
    std::mutex pool_mutex_;

public:
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
};

class DataStore : public IDataStore {
private:
    // High-performance hash map with better memory layout
    struct KeyValuePair {
        RedisValue value;
        uint64_t mod_count{0};
        std::chrono::steady_clock::time_point expiry_time;

        KeyValuePair() = default;
        KeyValuePair(RedisValue v) : value(std::move(v)) {}
    };

    std::unordered_map<std::string, KeyValuePair> data_;
    mutable std::mutex data_mutex_;
    MemoryPool memory_pool_;

    // Helper methods
    bool is_expired(const KeyValuePair& kvp) const noexcept;
    void cleanup_expired_keys();
    uint64_t generate_mod_count() noexcept;

public:
    DataStore();

    // IDataStore implementation
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


class StringHandler : public IStringHandler {
private:
    DataStore& store_;

public:
    explicit StringHandler(DataStore& store) : store_(store) {}

    std::string get(const std::string& key) override;
    bool set(const std::string& key, const std::string& value) override;
    size_t strlen(const std::string& key) override;
    std::string getrange(const std::string& key, int start, int end) override;
    size_t setrange(const std::string& key, size_t offset, const std::string& value) override;
};

class ListHandler : public IListHandler {
private:
    DataStore& store_;

public:
    explicit ListHandler(DataStore& store) : store_(store) {}

    size_t lpush(const std::string& key, const std::vector<std::string>& values) override;
    size_t rpush(const std::string& key, const std::vector<std::string>& values) override;
    std::string lpop(const std::string& key) override;
    std::string rpop(const std::string& key) override;
    size_t llen(const std::string& key) override;
    std::vector<std::string> lrange(const std::string& key, int start, int end) override;
    std::string lindex(const std::string& key, int index) override;
    bool lset(const std::string& key, int index, const std::string& value) override;
};

class SetHandler : public ISetHandler {
private:
    DataStore& store_;

public:
    explicit SetHandler(DataStore& store) : store_(store) {}

    size_t sadd(const std::string& key, const std::vector<std::string>& members) override;
    size_t srem(const std::string& key, const std::vector<std::string>& members) override;
    bool sismember(const std::string& key, const std::string& member) override;
    size_t scard(const std::string& key) override;
    std::vector<std::string> smembers(const std::string& key) override;
    std::vector<std::string> srandmember(const std::string& key, int count) override;
    std::string spop(const std::string& key) override;
};

class HashHandler : public IHashHandler {
private:
    DataStore& store_;

public:
    explicit HashHandler(DataStore& store) : store_(store) {}

    bool hset(const std::string& key, const std::string& field, const std::string& value) override;
    std::string hget(const std::string& key, const std::string& field) override;
    bool hdel(const std::string& key, const std::vector<std::string>& fields) override;
    size_t hlen(const std::string& key) override;
    std::vector<std::string> hkeys(const std::string& key) override;
    std::vector<std::string> hvals(const std::string& key) override;
    std::vector<std::pair<std::string, std::string>> hgetall(const std::string& key) override;
    bool hexists(const std::string& key, const std::string& field) override;
};

class SortedSetHandler : public ISortedSetHandler {
private:
    DataStore& store_;

public:
    explicit SortedSetHandler(DataStore& store) : store_(store) {}

    bool zadd(const std::string& key, const std::vector<std::pair<double, std::string>>& members) override;
    bool zrem(const std::string& key, const std::vector<std::string>& members) override;
    size_t zcard(const std::string& key) override;
    std::vector<std::string> zrange(const std::string& key, int start, int end, bool with_scores = false) override;
    std::vector<std::string> zrevrange(const std::string& key, int start, int end, bool with_scores = false) override;
    double zscore(const std::string& key, const std::string& member) override;
    long long zrank(const std::string& key, const std::string& member) override;
    long long zrevrank(const std::string& key, const std::string& member) override;
};

class PubSubManager : public IPubSubManager {
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

class WatchManager : public IWatchManager {
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