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