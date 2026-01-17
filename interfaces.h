#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

#include "redis_value.h"

class TransactionCommand;

class IDataStore {
public:
    virtual ~IDataStore() = default;

    virtual bool set(const std::string& key, const RedisValue& value) = 0;
    virtual std::unique_ptr<RedisValue> get(const std::string& key) = 0;
    virtual bool del(const std::string& key) = 0;
    virtual bool exists(const std::string& key) = 0;
    virtual std::vector<std::string> keys(const std::string& pattern = "*") = 0;
    virtual size_t dbsize() const = 0;
    virtual bool expire(const std::string& key, std::chrono::seconds duration) = 0;
    virtual bool pexpire(const std::string& key, std::chrono::milliseconds duration) = 0;
    virtual std::chrono::seconds ttl(const std::string& key) = 0;
    virtual std::chrono::milliseconds pttl(const std::string& key) = 0;
    virtual bool persist(const std::string& key) = 0;
    virtual uint64_t get_mod_count(const std::string& key) = 0;
    virtual void increment_mod_count(const std::string& key) = 0;
    virtual RedisType type(const std::string& key) = 0;
};

class IPubSubManager {
public:
    virtual ~IPubSubManager() = default;

    virtual void subscribe(int client_fd, const std::string& channel) = 0;
    virtual void unsubscribe(int client_fd, const std::string& channel) = 0;
    virtual void unsubscribe_all(int client_fd) = 0;
    virtual std::vector<int> get_subscribers(const std::string& channel) = 0;
    virtual void publish(const std::string& channel, const std::string& message) = 0;
};

class IPersistenceManager {
public:
    virtual ~IPersistenceManager() = default;

    virtual bool save_snapshot() = 0;
    virtual bool load_snapshot() = 0;
    virtual bool append_operation(const std::string& operation) = 0;
};

class ILuaScriptingEngine {
public:
    virtual ~ILuaScriptingEngine() = default;

    virtual std::string eval(const std::string& script,
                            const std::vector<std::string>& keys,
                            const std::vector<std::string>& args) = 0;
    virtual std::string evalsha(const std::string& sha1,
                               const std::vector<std::string>& keys,
                               const std::vector<std::string>& args) = 0;
    virtual bool script_exists(const std::string& sha1) = 0;
    virtual std::string script_load(const std::string& script) = 0;
    virtual void script_flush() = 0;
};

class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    virtual std::string handle(const std::vector<std::string>& args) = 0;
    virtual std::string command_name() const = 0;
    virtual size_t min_args() const = 0;
};

class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    virtual bool begin_transaction() = 0;
    virtual bool commit_transaction() = 0;
    virtual bool rollback_transaction() = 0;
    virtual bool is_in_transaction() const = 0;
    virtual void queue_command(const TransactionCommand& cmd) = 0;
    virtual std::vector<TransactionCommand> get_queued_commands() = 0;
};

class IWatchManager {
public:
    virtual ~IWatchManager() = default;

    virtual bool watch_key(const std::string& key, uint64_t expected_version) = 0;
    virtual bool unwatch_key(const std::string& key) = 0;
    virtual bool unwatch_all() = 0;
    virtual bool check_watched_keys() = 0;
    virtual void clear_watch_state() = 0;
};
class IStringHandler {
public:
    virtual ~IStringHandler() = default;
    virtual std::string get(const std::string& key) = 0;
    virtual bool set(const std::string& key, const std::string& value) = 0;
    virtual size_t strlen(const std::string& key) = 0;
    virtual std::string getrange(const std::string& key, int start, int end) = 0;
    virtual size_t setrange(const std::string& key, size_t offset, const std::string& value) = 0;
};

class IListHandler {
public:
    virtual ~IListHandler() = default;
    virtual size_t lpush(const std::string& key, const std::vector<std::string>& values) = 0;
    virtual size_t rpush(const std::string& key, const std::vector<std::string>& values) = 0;
    virtual std::string lpop(const std::string& key) = 0;
    virtual std::string rpop(const std::string& key) = 0;
    virtual size_t llen(const std::string& key) = 0;
    virtual std::vector<std::string> lrange(const std::string& key, int start, int end) = 0;
    virtual std::string lindex(const std::string& key, int index) = 0;
    virtual bool lset(const std::string& key, int index, const std::string& value) = 0;
};

class ISetHandler {
public:
    virtual ~ISetHandler() = default;
    virtual size_t sadd(const std::string& key, const std::vector<std::string>& members) = 0;
    virtual size_t srem(const std::string& key, const std::vector<std::string>& members) = 0;
    virtual bool sismember(const std::string& key, const std::string& member) = 0;
    virtual size_t scard(const std::string& key) = 0;
    virtual std::vector<std::string> smembers(const std::string& key) = 0;
    virtual std::vector<std::string> srandmember(const std::string& key, int count) = 0;
    virtual std::string spop(const std::string& key) = 0;
};

class IHashHandler {
public:
    virtual ~IHashHandler() = default;
    virtual bool hset(const std::string& key, const std::string& field, const std::string& value) = 0;
    virtual std::string hget(const std::string& key, const std::string& field) = 0;
    virtual bool hdel(const std::string& key, const std::vector<std::string>& fields) = 0;
    virtual size_t hlen(const std::string& key) = 0;
    virtual std::vector<std::string> hkeys(const std::string& key) = 0;
    virtual std::vector<std::string> hvals(const std::string& key) = 0;
    virtual std::vector<std::pair<std::string, std::string>> hgetall(const std::string& key) = 0;
    virtual bool hexists(const std::string& key, const std::string& field) = 0;
};

class ISortedSetHandler {
public:
    virtual ~ISortedSetHandler() = default;
    virtual bool zadd(const std::string& key, const std::vector<std::pair<double, std::string>>& members) = 0;
    virtual bool zrem(const std::string& key, const std::vector<std::string>& members) = 0;
    virtual size_t zcard(const std::string& key) = 0;
    virtual std::vector<std::string> zrange(const std::string& key, int start, int end, bool with_scores = false) = 0;
    virtual std::vector<std::string> zrevrange(const std::string& key, int start, int end, bool with_scores = false) = 0;
    virtual double zscore(const std::string& key, const std::string& member) = 0;
    virtual long long zrank(const std::string& key, const std::string& member) = 0;
    virtual long long zrevrank(const std::string& key, const std::string& member) = 0;
};