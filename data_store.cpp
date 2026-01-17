#include "data_store.h"
#include <algorithm>
#include <random>

void* MemoryPool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    // Simplified implementation - in production, implement proper memory pooling
    return new char[size];
}

void MemoryPool::deallocate(void* ptr, size_t size) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    delete[] static_cast<char*>(ptr);
}

DataStore::DataStore() = default;

bool DataStore::is_expired(const KeyValuePair& kvp) const noexcept {
    return kvp.expiry_time != std::chrono::steady_clock::time_point{} &&
           std::chrono::steady_clock::now() > kvp.expiry_time;
}

void DataStore::cleanup_expired_keys() {
    for (auto it = data_.begin(); it != data_.end(); ) {
        if (is_expired(it->second)) {
            it = data_.erase(it);
        } else {
            ++it;
        }
    }
}

uint64_t DataStore::generate_mod_count() noexcept {
    static std::atomic<uint64_t> counter{1};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

bool DataStore::set(const std::string& key, const RedisValue& value) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto& kvp = data_[key];
    kvp.value = value;
    kvp.mod_count = generate_mod_count();
    return true;
}

std::unique_ptr<RedisValue> DataStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) {
        return nullptr;
    }
    return std::make_unique<RedisValue>(it->second.value);
}

bool DataStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it != data_.end() && !is_expired(it->second)) {
        it->second.mod_count = generate_mod_count();
        data_.erase(it);
        return true;
    }
    return false;
}

bool DataStore::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    return it != data_.end() && !is_expired(it->second);
}

std::vector<std::string> DataStore::keys(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    cleanup_expired_keys();

    std::vector<std::string> result;
    for (const auto& [key, _] : data_) {
        if (pattern == "*" || key.find(pattern) != std::string::npos) {
            result.push_back(key);
        }
    }
    return result;
}

size_t DataStore::dbsize() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    size_t count = 0;
    for (const auto& [_, kvp] : data_) {
        if (!is_expired(kvp)) count++;
    }
    return count;
}

bool DataStore::expire(const std::string& key, std::chrono::seconds duration) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) return false;

    it->second.expiry_time = std::chrono::steady_clock::now() + duration;
    it->second.mod_count = generate_mod_count();
    return true;
}

bool DataStore::pexpire(const std::string& key, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) return false;

    it->second.expiry_time = std::chrono::steady_clock::now() + duration;
    it->second.mod_count = generate_mod_count();
    return true;
}

std::chrono::seconds DataStore::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) return std::chrono::seconds(-2);

    auto now = std::chrono::steady_clock::now();
    if (now >= it->second.expiry_time) return std::chrono::seconds(-2);

    return std::chrono::duration_cast<std::chrono::seconds>(it->second.expiry_time - now);
}

std::chrono::milliseconds DataStore::pttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) return std::chrono::milliseconds(-2);

    auto now = std::chrono::steady_clock::now();
    if (now >= it->second.expiry_time) return std::chrono::milliseconds(-2);

    return std::chrono::duration_cast<std::chrono::milliseconds>(it->second.expiry_time - now);
}

bool DataStore::persist(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) return false;

    it->second.expiry_time = std::chrono::steady_clock::time_point{};
    it->second.mod_count = generate_mod_count();
    return true;
}

uint64_t DataStore::get_mod_count(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second.mod_count : 0;
}

void DataStore::increment_mod_count(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        it->second.mod_count = generate_mod_count();
    }
}

RedisType DataStore::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || is_expired(it->second)) return RedisType::String;
    return it->second.value.type;
}

// String handler implementation
std::string StringHandler::get(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::String ? value->str_val : "";
}

bool StringHandler::set(const std::string& key, const std::string& value) {
    RedisValue rv(RedisType::String);
    rv.str_val = value;
    return store_.set(key, rv);
}

size_t StringHandler::strlen(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::String ? value->str_val.length() : 0;
}

std::string StringHandler::getrange(const std::string& key, int start, int end) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::String) return "";

    const std::string& str = value->str_val;
    size_t len = str.length();

    // Handle negative indices
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    start = std::max(0, start);
    end = std::min(static_cast<int>(len) - 1, end);

    if (start > end) return "";
    return str.substr(start, end - start + 1);
}

size_t StringHandler::setrange(const std::string& key, size_t offset, const std::string& value) {
    auto current_value = store_.get(key);
    std::string str = current_value && current_value->type == RedisType::String ? current_value->str_val : "";

    if (offset + value.length() > str.length()) {
        str.resize(offset + value.length());
    }

    str.replace(offset, value.length(), value);
    store_.set(key, RedisValue(str));
    return str.length();
}

// Simplified implementations for other handlers (focus on correctness first)
size_t ListHandler::lpush(const std::string& key, const std::vector<std::string>& values) {
    auto current_value = store_.get(key);
    std::vector<std::string> list = current_value && current_value->type == RedisType::List ?
                                   current_value->list_val : std::vector<std::string>{};

    list.insert(list.begin(), values.begin(), values.end());
    RedisValue rv(RedisType::List);
    rv.list_val = list;
    store_.set(key, rv);
    return list.size();
}

size_t ListHandler::rpush(const std::string& key, const std::vector<std::string>& values) {
    auto current_value = store_.get(key);
    std::vector<std::string> list = current_value && current_value->type == RedisType::List ?
                                   current_value->list_val : std::vector<std::string>{};

    list.insert(list.end(), values.begin(), values.end());
    store_.set(key, RedisValue(list));
    return list.size();
}

std::string ListHandler::lpop(const std::string& key) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::List || current_value->list_val.empty()) {
        return "";
    }

    std::vector<std::string> list = current_value->list_val;
    std::string result = list.front();
    list.erase(list.begin());

    if (list.empty()) {
        store_.del(key);
    } else {
        store_.set(key, RedisValue(list));
    }
    return result;
}

std::string ListHandler::rpop(const std::string& key) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::List || current_value->list_val.empty()) {
        return "";
    }

    std::vector<std::string> list = current_value->list_val;
    std::string result = list.back();
    list.pop_back();

    if (list.empty()) {
        store_.del(key);
    } else {
        store_.set(key, RedisValue(list));
    }
    return result;
}

size_t ListHandler::llen(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::List ? value->list_val.size() : 0;
}

std::vector<std::string> ListHandler::lrange(const std::string& key, int start, int end) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::List) return {};

    const auto& list = value->list_val;
    size_t len = list.size();

    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    start = std::max(0, start);
    end = std::min(static_cast<int>(len) - 1, end);

    if (start > end) return {};

    return std::vector<std::string>(list.begin() + start, list.begin() + end + 1);
}

std::string ListHandler::lindex(const std::string& key, int index) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::List) return "";

    const auto& list = value->list_val;
    size_t len = list.size();

    if (index < 0) index = len + index;
    if (index < 0 || static_cast<size_t>(index) >= len) return "";

    return list[index];
}

bool ListHandler::lset(const std::string& key, int index, const std::string& value) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::List) return false;

    auto list = current_value->list_val;
    size_t len = list.size();

    if (index < 0) index = len + index;
    if (index < 0 || static_cast<size_t>(index) >= len) return false;

    list[index] = value;
    store_.set(key, RedisValue(list));
    return true;
}

// PubSub Manager implementation
void PubSubManager::subscribe(int client_fd, const std::string& channel) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_[channel].push_back(client_fd);
}

void PubSubManager::unsubscribe(int client_fd, const std::string& channel) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto& subs = subscriptions_[channel];
    subs.erase(std::remove(subs.begin(), subs.end(), client_fd), subs.end());
    if (subs.empty()) {
        subscriptions_.erase(channel);
    }
}

void PubSubManager::unsubscribe_all(int client_fd) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    for (auto& [channel, subs] : subscriptions_) {
        subs.erase(std::remove(subs.begin(), subs.end(), client_fd), subs.end());
    }

    // Remove empty channels
    for (auto it = subscriptions_.begin(); it != subscriptions_.end(); ) {
        if (it->second.empty()) {
            it = subscriptions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<int> PubSubManager::get_subscribers(const std::string& channel) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(channel);
    return it != subscriptions_.end() ? it->second : std::vector<int>{};
}

void PubSubManager::publish(const std::string& channel, const std::string& message) {
    // For now, just return - actual publishing will be handled by the command processor
    // This maintains the interface for future enhancements
}

// Watch Manager implementation
bool WatchManager::watch_key(const std::string& key, uint64_t expected_version) {
    std::lock_guard<std::mutex> lock(watch_mutex_);
    watched_keys_[key][0] = expected_version; // Simplified - using 0 as client ID for now
    client_watches_[0].push_back(key);
    return true;
}

bool WatchManager::unwatch_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(watch_mutex_);
    watched_keys_.erase(key);
    for (auto& [client, keys] : client_watches_) {
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
    }
    return true;
}

bool WatchManager::unwatch_all() {
    std::lock_guard<std::mutex> lock(watch_mutex_);
    watched_keys_.clear();
    client_watches_.clear();
    return true;
}

bool WatchManager::check_watched_keys() {
    // Simplified implementation - in production, this would check if any watched keys changed
    return true;
}

void WatchManager::clear_watch_state() {
    std::lock_guard<std::mutex> lock(watch_mutex_);
    watched_keys_.clear();
    client_watches_.clear();
}

// Placeholder implementations for remaining handlers (focus on getting a working system first)
size_t SetHandler::sadd(const std::string& key, const std::vector<std::string>& members) {
    auto current_value = store_.get(key);
    std::unordered_set<std::string> set = current_value && current_value->type == RedisType::Set ?
                                         std::unordered_set<std::string>(current_value->set_val.begin(), current_value->set_val.end()) :
                                         std::unordered_set<std::string>{};

    size_t added = 0;
    for (const auto& member : members) {
        if (set.insert(member).second) added++;
    }

    std::vector<std::string> set_vec(set.begin(), set.end());
    store_.set(key, RedisValue(set_vec, RedisType::Set));
    return added;
}

size_t SetHandler::srem(const std::string& key, const std::vector<std::string>& members) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::Set) return 0;

    std::unordered_set<std::string> set(current_value->set_val.begin(), current_value->set_val.end());
    size_t removed = 0;

    for (const auto& member : members) {
        if (set.erase(member)) removed++;
    }

    if (set.empty()) {
        store_.del(key);
    } else {
        std::vector<std::string> set_vec(set.begin(), set.end());
        store_.set(key, RedisValue(set_vec, RedisType::Set));
    }
    return removed;
}

bool SetHandler::sismember(const std::string& key, const std::string& member) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::Set) return false;

    std::unordered_set<std::string> set(value->set_val.begin(), value->set_val.end());
    return set.count(member) > 0;
}

size_t SetHandler::scard(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::Set ? value->set_val.size() : 0;
}

std::vector<std::string> SetHandler::smembers(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::Set ? value->set_val : std::vector<std::string>{};
}

std::vector<std::string> SetHandler::srandmember(const std::string& key, int count) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::Set || value->set_val.empty()) return {};

    const auto& members = value->set_val;
    std::vector<std::string> result;

    if (count >= 0) {
        count = std::min(count, static_cast<int>(members.size()));
        std::sample(members.begin(), members.end(), std::back_inserter(result), count,
                   std::mt19937{std::random_device{}()});
    } else {
        // Negative count means allow duplicates (but we don't support duplicates in sets)
        count = std::min(-count, static_cast<int>(members.size()));
        std::sample(members.begin(), members.end(), std::back_inserter(result), count,
                   std::mt19937{std::random_device{}()});
    }

    return result;
}

std::string SetHandler::spop(const std::string& key) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::Set || current_value->set_val.empty()) {
        return "";
    }

    auto members = current_value->set_val;
    std::string result = members.back();
    members.pop_back();

    if (members.empty()) {
        store_.del(key);
    } else {
        store_.set(key, RedisValue(members, RedisType::Set));
    }
    return result;
}

// Hash operations
bool HashHandler::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto current_value = store_.get(key);
    std::unordered_map<std::string, std::string> hash = current_value && current_value->type == RedisType::Hash ?
                                                        std::unordered_map<std::string, std::string>(current_value->hash_val.begin(), current_value->hash_val.end()) :
                                                        std::unordered_map<std::string, std::string>{};

    bool is_new = hash.find(field) == hash.end();
    hash[field] = value;
    store_.set(key, RedisValue(hash));
    return is_new;
}

std::string HashHandler::hget(const std::string& key, const std::string& field) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::Hash) return "";

    auto it = value->hash_val.find(field);
    return it != value->hash_val.end() ? it->second : "";
}

bool HashHandler::hdel(const std::string& key, const std::vector<std::string>& fields) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::Hash) return false;

    auto hash = current_value->hash_val;
    bool deleted = false;

    for (const auto& field : fields) {
        if (hash.erase(field)) deleted = true;
    }

    if (hash.empty()) {
        store_.del(key);
    } else {
        store_.set(key, RedisValue(hash));
    }
    return deleted;
}

size_t HashHandler::hlen(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::Hash ? value->hash_val.size() : 0;
}

std::vector<std::string> HashHandler::hkeys(const std::string& key) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::Hash) return {};

    std::vector<std::string> keys;
    for (const auto& [k, _] : value->hash_val) {
        keys.push_back(k);
    }
    return keys;
}

std::vector<std::string> HashHandler::hvals(const std::string& key) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::Hash) return {};

    std::vector<std::string> vals;
    for (const auto& [_, v] : value->hash_val) {
        vals.push_back(v);
    }
    return vals;
}

std::vector<std::pair<std::string, std::string>> HashHandler::hgetall(const std::string& key) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::Hash) return {};

    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& [k, v] : value->hash_val) {
        result.emplace_back(k, v);
    }
    return result;
}

bool HashHandler::hexists(const std::string& key, const std::string& field) {
    auto value = store_.get(key);
    return value && value->type == RedisType::Hash && value->hash_val.count(field) > 0;
}

// Sorted Set operations
bool SortedSetHandler::zadd(const std::string& key, const std::vector<std::pair<double, std::string>>& members) {
    auto current_value = store_.get(key);
    std::map<double, std::string> score_to_member;
    std::unordered_map<std::string, double> member_to_score;

    if (current_value && current_value->type == RedisType::ZSet) {
        for (const auto& [score, member] : current_value->zset_score_to_member) {
            score_to_member[score] = member;
            member_to_score[member] = score;
        }
    }

    for (const auto& [score, member] : members) {
        if (member_to_score.count(member)) {
            // Remove old score
            score_to_member.erase(member_to_score[member]);
        }
        score_to_member[score] = member;
        member_to_score[member] = score;
    }

    std::vector<std::pair<double, std::string>> zset_data;
    for (const auto& [score, member] : score_to_member) {
        zset_data.emplace_back(score, member);
    }

    store_.set(key, RedisValue(zset_data, RedisType::ZSet));
    return true;
}

bool SortedSetHandler::zrem(const std::string& key, const std::vector<std::string>& members) {
    auto current_value = store_.get(key);
    if (!current_value || current_value->type != RedisType::ZSet) return false;

    std::map<double, std::string> score_to_member;
    std::unordered_map<std::string, double> member_to_score;

    for (const auto& [score, member] : current_value->zset_score_to_member) {
        score_to_member[score] = member;
        member_to_score[member] = score;
    }

    bool removed = false;
    for (const auto& member : members) {
        if (member_to_score.count(member)) {
            score_to_member.erase(member_to_score[member]);
            member_to_score.erase(member);
            removed = true;
        }
    }

    if (score_to_member.empty()) {
        store_.del(key);
    } else {
        std::vector<std::pair<double, std::string>> zset_data;
        for (const auto& [score, member] : score_to_member) {
            zset_data.emplace_back(score, member);
        }
        store_.set(key, RedisValue(zset_data, RedisType::ZSet));
    }
    return removed;
}

size_t SortedSetHandler::zcard(const std::string& key) {
    auto value = store_.get(key);
    return value && value->type == RedisType::ZSet ? value->zset_member_to_score.size() : 0;
}

std::vector<std::string> SortedSetHandler::zrange(const std::string& key, int start, int end, bool with_scores) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::ZSet) return {};

    const auto& score_to_member = value->zset_score_to_member;
    size_t len = score_to_member.size();

    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    start = std::max(0, start);
    end = std::min(static_cast<int>(len) - 1, end);

    if (start > end) return {};

    std::vector<std::string> result;
    auto it = score_to_member.begin();
    std::advance(it, start);

    for (int i = start; i <= end && it != score_to_member.end(); ++i, ++it) {
        result.push_back(it->second);
        if (with_scores) {
            result.push_back(std::to_string(it->first));
        }
    }

    return result;
}

std::vector<std::string> SortedSetHandler::zrevrange(const std::string& key, int start, int end, bool with_scores) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::ZSet) return {};

    const auto& score_to_member = value->zset_score_to_member;
    size_t len = score_to_member.size();

    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    start = std::max(0, start);
    end = std::min(static_cast<int>(len) - 1, end);

    if (start > end) return {};

    std::vector<std::string> result;
    auto it = score_to_member.rbegin();
    std::advance(it, start);

    for (int i = start; i <= end && it != score_to_member.rend(); ++i, ++it) {
        result.push_back(it->second);
        if (with_scores) {
            result.push_back(std::to_string(it->first));
        }
    }

    return result;
}

double SortedSetHandler::zscore(const std::string& key, const std::string& member) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::ZSet) return 0.0;

    auto it = value->zset_member_to_score.find(member);
    return it != value->zset_member_to_score.end() ? it->second : 0.0;
}

long long SortedSetHandler::zrank(const std::string& key, const std::string& member) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::ZSet) return -1;

    if (value->zset_member_to_score.find(member) == value->zset_member_to_score.end()) {
        return -1;
    }

    long long rank = 0;
    for (const auto& [score, mem] : value->zset_score_to_member) {
        if (mem == member) return rank;
        rank++;
    }
    return -1;
}

long long SortedSetHandler::zrevrank(const std::string& key, const std::string& member) {
    auto value = store_.get(key);
    if (!value || value->type != RedisType::ZSet) return -1;

    if (value->zset_member_to_score.find(member) == value->zset_member_to_score.end()) {
        return -1;
    }

    long long rank = value->zset_score_to_member.size() - 1;
    for (auto it = value->zset_score_to_member.rbegin(); it != value->zset_score_to_member.rend(); ++it) {
        if (it->second == member) return rank;
        rank--;
    }
    return -1;
}