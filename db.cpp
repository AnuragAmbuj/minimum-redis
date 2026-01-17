//
// Created by Sisyphus on 16/01/26.
//

#include "db.h"
#include "redis_value.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// String operations
bool Database::set(const std::string& key, const std::string& value) {
    auto& val = data[key];
    val.type = RedisType::String;
    val.str_val = value;
    return true;
}

std::string Database::get(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::String) {
        if (it->second.is_expired()) {
            data.erase(it);
            return "";
        }
        return it->second.str_val;
    }
    return ""; // Empty string indicates key not found
}

bool Database::del(const std::string& key) {
    return data.erase(key) > 0;
}

bool Database::exists(const std::string& key) {
    auto it = data.find(key);
    if (it == data.end()) return false;

    if (it->second.is_expired()) {
        data.erase(it);
        return false;
    }

    return true;
}

RedisType Database::type(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end()) {
        if (it->second.is_expired()) {
            data.erase(it);
            return RedisType::String; // Default type for non-existent
        }
        return it->second.type;
    }
    return RedisType::String; // Default type
}

std::vector<std::string> Database::keys() {
    std::vector<std::string> result;
    result.reserve(data.size());
    for (const auto& pair : data) {
        result.push_back(pair.first);
    }
    return result;
}

// Expiration operations
bool Database::expire(const std::string& key, int64_t seconds) {
    auto it = data.find(key);
    if (it == data.end()) return false;

    if (it->second.is_expired()) {
        data.erase(it);
        return false;
    }

    it->second.set_expiry_seconds(seconds);
    return true;
}

bool Database::pexpire(const std::string& key, int64_t milliseconds) {
    auto it = data.find(key);
    if (it == data.end()) return false;

    if (it->second.is_expired()) {
        data.erase(it);
        return false;
    }

    it->second.set_expiry_milliseconds(milliseconds);
    return true;
}

int64_t Database::ttl(const std::string& key) {
    auto it = data.find(key);
    if (it == data.end()) return -2; // Key doesn't exist

    if (it->second.is_expired()) {
        data.erase(it);
        return -2; // Key expired and was removed
    }

    return it->second.ttl_seconds();
}

int64_t Database::pttl(const std::string& key) {
    auto it = data.find(key);
    if (it == data.end()) return -2; // Key doesn't exist

    if (it->second.is_expired()) {
        data.erase(it);
        return -2; // Key expired and was removed
    }

    return it->second.ttl_milliseconds();
}

bool Database::persist(const std::string& key) {
    auto it = data.find(key);
    if (it == data.end()) return false;

    if (it->second.is_expired()) {
        data.erase(it);
        return false;
    }

    it->second.persist();
    return true;
}

// List operations
size_t Database::lpush(const std::string& key, const std::vector<std::string>& values) {
    auto& val = data[key];
    val.type = RedisType::List;
    val.list_val.insert(val.list_val.begin(), values.rbegin(), values.rend());
    return val.list_val.size();
}

size_t Database::rpush(const std::string& key, const std::vector<std::string>& values) {
    auto& val = data[key];
    val.type = RedisType::List;
    val.list_val.insert(val.list_val.end(), values.begin(), values.end());
    return val.list_val.size();
}

std::string Database::lpop(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::List && !it->second.list_val.empty()) {
        std::string result = it->second.list_val.front();
        it->second.list_val.erase(it->second.list_val.begin());
        return result;
    }
    return "";
}

std::string Database::rpop(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::List && !it->second.list_val.empty()) {
        std::string result = it->second.list_val.back();
        it->second.list_val.pop_back();
        return result;
    }
    return "";
}

size_t Database::llen(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::List) {
        if (it->second.is_expired()) {
            data.erase(it);
            return 0;
        }
        return it->second.list_val.size();
    }
    return 0;
}

std::vector<std::string> Database::lrange(const std::string& key, int start, int end) {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::List) {
        const auto& list = it->second.list_val;
        size_t size = list.size();

        // Handle negative indices
        if (start < 0) start = size + start;
        if (end < 0) end = size + end;

        start = std::max(0, start);
        end = std::min(static_cast<int>(size) - 1, end);

        if (start <= end) {
            result.assign(list.begin() + start, list.begin() + end + 1);
        }
    }
    return result;
}

std::string Database::lindex(const std::string& key, int index) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::List) {
        const auto& list = it->second.list_val;
        size_t size = list.size();

        if (index < 0) index = size + index;

        if (index >= 0 && static_cast<size_t>(index) < size) {
            return list[index];
        }
    }
    return "";
}

// Set operations
size_t Database::sadd(const std::string& key, const std::vector<std::string>& members) {
    auto& val = data[key];
    val.type = RedisType::Set;
    size_t before = val.set_val.size();
    val.set_val.insert(members.begin(), members.end());
    return val.set_val.size() - before;
}

size_t Database::srem(const std::string& key, const std::vector<std::string>& members) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Set) {
        size_t before = it->second.set_val.size();
        for (const auto& member : members) {
            it->second.set_val.erase(member);
        }
        return before - it->second.set_val.size();
    }
    return 0;
}

bool Database::sismember(const std::string& key, const std::string& member) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Set) {
        return it->second.set_val.count(member) > 0;
    }
    return false;
}

size_t Database::scard(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Set) {
        return it->second.set_val.size();
    }
    return 0;
}

std::vector<std::string> Database::smembers(const std::string& key) {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Set) {
        result.assign(it->second.set_val.begin(), it->second.set_val.end());
    }
    return result;
}

std::vector<std::string> Database::sinter(const std::vector<std::string>& keys) {
    std::vector<std::string> result;
    if (keys.empty()) return result;

    // Start with the first set
    auto it = data.find(keys[0]);
    if (it == data.end() || it->second.type != RedisType::Set) return result;

    result.assign(it->second.set_val.begin(), it->second.set_val.end());

    // Intersect with remaining sets
    for (size_t i = 1; i < keys.size(); ++i) {
        it = data.find(keys[i]);
        if (it == data.end() || it->second.type != RedisType::Set) {
            result.clear();
            return result;
        }

        std::vector<std::string> intersection;
        const auto& set2 = it->second.set_val;
        for (const auto& elem : result) {
            if (set2.count(elem)) {
                intersection.push_back(elem);
            }
        }
        result = std::move(intersection);
    }

    return result;
}

std::vector<std::string> Database::sunion(const std::vector<std::string>& keys) {
    std::unordered_set<std::string> union_set;

    for (const auto& key : keys) {
        auto it = data.find(key);
        if (it != data.end() && it->second.type == RedisType::Set) {
            union_set.insert(it->second.set_val.begin(), it->second.set_val.end());
        }
    }

    return std::vector<std::string>(union_set.begin(), union_set.end());
}

std::vector<std::string> Database::sdiff(const std::vector<std::string>& keys) {
    std::vector<std::string> result;
    if (keys.empty()) return result;

    // Start with the first set
    auto it = data.find(keys[0]);
    if (it == data.end() || it->second.type != RedisType::Set) return result;

    std::unordered_set<std::string> diff_set(it->second.set_val.begin(), it->second.set_val.end());

    // Remove elements from subsequent sets
    for (size_t i = 1; i < keys.size(); ++i) {
        it = data.find(keys[i]);
        if (it != data.end() && it->second.type == RedisType::Set) {
            for (const auto& elem : it->second.set_val) {
                diff_set.erase(elem);
            }
        }
    }

    result.assign(diff_set.begin(), diff_set.end());
    return result;
}

// Hash operations
bool Database::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto& val = data[key];
    val.type = RedisType::Hash;
    val.hash_val[field] = value;
    return true;
}

std::string Database::hget(const std::string& key, const std::string& field) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Hash) {
        auto field_it = it->second.hash_val.find(field);
        if (field_it != it->second.hash_val.end()) {
            return field_it->second;
        }
    }
    return "";
}

bool Database::hdel(const std::string& key, const std::vector<std::string>& fields) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Hash) {
        size_t before = it->second.hash_val.size();
        for (const auto& field : fields) {
            it->second.hash_val.erase(field);
        }
        return it->second.hash_val.size() < before;
    }
    return false;
}

size_t Database::hlen(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Hash) {
        return it->second.hash_val.size();
    }
    return 0;
}

std::vector<std::string> Database::hkeys(const std::string& key) {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Hash) {
        for (const auto& pair : it->second.hash_val) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::vector<std::string> Database::hvals(const std::string& key) {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Hash) {
        for (const auto& pair : it->second.hash_val) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::unordered_map<std::string, std::string> Database::hgetall(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::Hash) {
        return it->second.hash_val;
    }
    return {};
}

// Sorted Set operations
bool Database::zadd(const std::string& key, const std::vector<std::pair<double, std::string>>& members) {
    auto& val = data[key];
    val.type = RedisType::ZSet;

    bool added = false;
    for (const auto& [score, member] : members) {
        // Remove old score if exists
        auto old_it = val.zset_member_to_score.find(member);
        if (old_it != val.zset_member_to_score.end()) {
            // Remove from multimap - need to find the specific entry
            auto range = val.zset_score_to_member.equal_range(old_it->second);
            auto it = range.first;
            while (it != range.second) {
                if (it->second == member) {
                    it = val.zset_score_to_member.erase(it);
                    break;
                } else {
                    ++it;
                }
            }
        }

        // Add new score
        val.zset_member_to_score[member] = score;
        val.zset_score_to_member.insert({score, member});
        added = true;
    }
    return added;
}

bool Database::zrem(const std::string& key, const std::vector<std::string>& members) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::ZSet) {
        bool removed = false;
        for (const auto& member : members) {
            auto score_it = it->second.zset_member_to_score.find(member);
            if (score_it != it->second.zset_member_to_score.end()) {
                double score = score_it->second;
                it->second.zset_member_to_score.erase(member);
                auto range = it->second.zset_score_to_member.equal_range(score);
                auto map_it = range.first;
                while (map_it != range.second) {
                    if (map_it->second == member) {
                        map_it = it->second.zset_score_to_member.erase(map_it);
                        break;
                    } else {
                        ++map_it;
                    }
                }
                removed = true;
            }
        }
        return removed;
    }
    return false;
}

size_t Database::zcard(const std::string& key) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::ZSet) {
        return it->second.zset_member_to_score.size();
    }
    return 0;
}

std::vector<std::string> Database::zrange(const std::string& key, int start, int end) {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::ZSet) {
        const auto& score_map = it->second.zset_score_to_member;
        size_t size = score_map.size();

        if (start < 0) start = size + start;
        if (end < 0) end = size + end;

        start = std::max(0, start);
        end = std::min(static_cast<int>(size) - 1, end);

        if (start <= end) {
            auto map_it = score_map.begin();
            std::advance(map_it, start);
            for (int i = start; i <= end && map_it != score_map.end(); ++i, ++map_it) {
                result.push_back(map_it->second);
            }
        }
    }
    return result;
}

std::vector<std::string> Database::zrevrange(const std::string& key, int start, int end) {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::ZSet) {
        const auto& score_map = it->second.zset_score_to_member;
        size_t size = score_map.size();

        if (start < 0) start = size + start;
        if (end < 0) end = size + end;

        start = std::max(0, start);
        end = std::min(static_cast<int>(size) - 1, end);

        if (start <= end) {
            auto map_it = score_map.rbegin();
            std::advance(map_it, start);
            for (int i = start; i <= end && map_it != score_map.rend(); ++i, ++map_it) {
                result.push_back(map_it->second);
            }
        }
    }
    return result;
}

double Database::zscore(const std::string& key, const std::string& member) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::ZSet) {
        auto score_it = it->second.zset_member_to_score.find(member);
        if (score_it != it->second.zset_member_to_score.end()) {
            return score_it->second;
        }
    }
    return 0.0; // Should return nil, but for simplicity
}

long long Database::zrank(const std::string& key, const std::string& member) {
    auto it = data.find(key);
    if (it != data.end() && it->second.type == RedisType::ZSet) {
        auto score_it = it->second.zset_member_to_score.find(member);
        if (score_it != it->second.zset_member_to_score.end()) {
            double score = score_it->second;
            // Count members with lower scores
            long long rank = 0;
            for (const auto& pair : it->second.zset_score_to_member) {
                if (pair.first < score) {
                    ++rank;
                } else if (pair.first == score && pair.second < member) {
                    ++rank;
                } else if (pair.first == score && pair.second == member) {
                    break;
                }
            }
            return rank;
        }
    }
    return -1; // Not found
}

// TODO: Implement persistence for all data types
bool Database::save_to_file(const std::string& filename) {
    // Temporary implementation - only saves strings
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& pair : data) {
        if (pair.second.type == RedisType::String) {
            file << "string:" << pair.first << "=" << pair.second.str_val << "\n";
        }
        // TODO: Add other types
    }

    file.close();
    return true;
}

bool Database::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false; // File doesn't exist or can't be opened
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type_str, key, value;
        if (std::getline(iss, type_str, ':') && std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (type_str == "string") {
                set(key, value);
            }
            // TODO: Add other types
        }
    }

    file.close();
    return true;
}