//
// Created by Sisyphus on 16/01/26.
//

#ifndef MINIMALREDIS_DB_H
#define MINIMALREDIS_DB_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include "redis_value.h"

class Database {
private:
    std::unordered_map<std::string, RedisValue> data;

public:
    // String operations
    bool set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);

    // Generic operations
    std::vector<std::string> keys();
    RedisType type(const std::string& key);

    // List operations
    size_t lpush(const std::string& key, const std::vector<std::string>& values);
    size_t rpush(const std::string& key, const std::vector<std::string>& values);
    std::string lpop(const std::string& key);
    std::string rpop(const std::string& key);
    size_t llen(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, int start, int end);
    std::string lindex(const std::string& key, int index);

    // Set operations
    size_t sadd(const std::string& key, const std::vector<std::string>& members);
    size_t srem(const std::string& key, const std::vector<std::string>& members);
    bool sismember(const std::string& key, const std::string& member);
    size_t scard(const std::string& key);
    std::vector<std::string> smembers(const std::string& key);
    std::vector<std::string> sinter(const std::vector<std::string>& keys);
    std::vector<std::string> sunion(const std::vector<std::string>& keys);
    std::vector<std::string> sdiff(const std::vector<std::string>& keys);

    // Hash operations
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    std::string hget(const std::string& key, const std::string& field);
    bool hdel(const std::string& key, const std::vector<std::string>& fields);
    size_t hlen(const std::string& key);
    std::vector<std::string> hkeys(const std::string& key);
    std::vector<std::string> hvals(const std::string& key);
    std::unordered_map<std::string, std::string> hgetall(const std::string& key);

    // Sorted Set operations
    bool zadd(const std::string& key, const std::vector<std::pair<double, std::string>>& members);
    bool zrem(const std::string& key, const std::vector<std::string>& members);
    size_t zcard(const std::string& key);
    std::vector<std::string> zrange(const std::string& key, int start, int end);
    std::vector<std::string> zrevrange(const std::string& key, int start, int end);
    double zscore(const std::string& key, const std::string& member);
    long long zrank(const std::string& key, const std::string& member);

    bool save_to_file(const std::string& filename);
    bool load_from_file(const std::string& filename);
};

#endif //MINIMALREDIS_DB_H