//
// Created by Sisyphus on 16/01/26.
//

#ifndef MINIMALREDIS_REDIS_VALUE_H
#define MINIMALREDIS_REDIS_VALUE_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <variant>
#include <chrono>

enum class RedisType {
    String,
    List,
    Set,
    Hash,
    ZSet
};

class RedisValue {
public:
    RedisType type;

    // Expiration timestamp (milliseconds since epoch, 0 means no expiration)
    int64_t expiry_time = 0;

    // Modification counter for optimistic concurrency (WATCH/UNWATCH)
    uint64_t mod_count = 0;

    // Storage for different types
    std::string str_val;
    std::vector<std::string> list_val;
    std::unordered_set<std::string> set_val;
    std::unordered_map<std::string, std::string> hash_val;
    // For ZSet: score -> member (for ordered operations), and member -> score (for lookups)
    // Using multimap to handle duplicate scores, ordered by score then lexicographically
    std::multimap<double, std::string> zset_score_to_member;
    std::unordered_map<std::string, double> zset_member_to_score;

    RedisValue(RedisType t = RedisType::String) : type(t) {}

    // Check if key has expired
    bool is_expired() const {
        if (expiry_time == 0) return false;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return now >= expiry_time;
    }

    // Set expiration time (milliseconds since epoch)
    void set_expiry(int64_t timestamp_ms) {
        expiry_time = timestamp_ms;
    }

    // Set expiration time from seconds
    void set_expiry_seconds(int64_t seconds) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        expiry_time = now + (seconds * 1000);
    }

    // Set expiration time from milliseconds
    void set_expiry_milliseconds(int64_t milliseconds) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        expiry_time = now + milliseconds;
    }

    // Get remaining TTL in seconds (-1 if no expiry, -2 if expired)
    int64_t ttl_seconds() const {
        if (expiry_time == 0) return -1;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now >= expiry_time) return -2;
        return (expiry_time - now) / 1000;
    }

    // Get remaining TTL in milliseconds (-1 if no expiry, -2 if expired)
    int64_t ttl_milliseconds() const {
        if (expiry_time == 0) return -1;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now >= expiry_time) return -2;
        return expiry_time - now;
    }

    // Remove expiration
    void persist() {
        expiry_time = 0;
    }

    // Helper methods
    void clear() {
        str_val.clear();
        list_val.clear();
        set_val.clear();
        hash_val.clear();
        zset_score_to_member.clear();
        zset_member_to_score.clear();
    }

    size_t size() const {
        switch (type) {
            case RedisType::String: return str_val.size();
            case RedisType::List: return list_val.size();
            case RedisType::Set: return set_val.size();
            case RedisType::Hash: return hash_val.size();
            case RedisType::ZSet: return zset_member_to_score.size();
            default: return 0;
        }
    }
};

#endif //MINIMALREDIS_REDIS_VALUE_H