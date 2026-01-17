#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

class RedisClient;

class DataBrowser {
private:
    RedisClient& client;
    std::vector<std::pair<std::string, std::string>> key_value_pairs;
    std::string selected_key;
    std::string filter_pattern;
    int selected_index = 0;
    bool loading = false;

    void refresh_data();
    void load_key_details(const std::string& key);

public:
    DataBrowser(RedisClient& c);
    ftxui::Component render();
    void update();
};

class CommandInterface {
private:
    RedisClient& client;
    std::vector<std::string> command_history;
    std::vector<std::string> response_history;
    std::string current_command;
    int history_index = -1;
    int selected_history = -1;
    bool show_suggestions = false;
    std::vector<std::string> suggestions;

    std::vector<std::string> redis_commands = {
        "SET", "GET", "DEL", "EXISTS", "KEYS", "TYPE", "RENAME",
        "LPUSH", "RPUSH", "LPOP", "RPOP", "LLEN", "LRANGE", "LINDEX",
        "SADD", "SREM", "SISMEMBER", "SCARD", "SMEMBERS", "SUNION", "SINTER",
        "HSET", "HGET", "HDEL", "HLEN", "HKEYS", "HVALS", "HGETALL",
        "ZADD", "ZREM", "ZCARD", "ZRANGE", "ZREVRANGE", "ZSCORE", "ZRANK",
        "MULTI", "EXEC", "DISCARD", "WATCH", "UNWATCH",
        "EXPIRE", "PEXPIRE", "TTL", "PTTL", "PERSIST",
        "SUBSCRIBE", "PUBLISH", "UNSUBSCRIBE", "PSUBSCRIBE", "PUNSUBSCRIBE",
        "EVAL", "EVALSHA", "SCRIPT",
        "SAVE", "BGSAVE", "LASTSAVE", "FLUSHDB", "FLUSHALL", "DBSIZE",
        "INFO", "CLIENT", "CONFIG", "DEBUG", "COMMAND", "MEMORY"
    };

    void execute_command();
    void update_suggestions();
    std::string get_command_preview();

public:
    CommandInterface(RedisClient& c);
    ftxui::Component render();
    void add_to_history(const std::string& command, const std::string& response);
};

class ServerMonitor {
private:
    RedisClient& client;
    std::unordered_map<std::string, std::string> server_info;
    std::vector<std::pair<std::string, double>> memory_history;
    std::vector<std::pair<std::string, double>> connections_history;
    bool auto_refresh = true;
    int refresh_interval = 2;

    void fetch_server_info();
    void update_metrics();

public:
    ServerMonitor(RedisClient& c);
    ftxui::Component render();
    void update();
};

class PubSubManager {
private:
    RedisClient& client;
    std::vector<std::string> subscribed_channels;
    std::vector<std::pair<std::string, std::string>> recent_messages;
    std::string new_channel;
    bool in_pubsub_mode = false;

    void subscribe_to_channel(const std::string& channel);
    void unsubscribe_from_channel(const std::string& channel);

public:
    PubSubManager(RedisClient& c);
    ftxui::Component render();
    void add_message(const std::string& channel, const std::string& message);
    bool is_in_pubsub_mode() const { return in_pubsub_mode; }
};

class ScriptEditor {
private:
    RedisClient& client;
    std::string current_script;
    std::vector<std::string> script_history;
    std::string execution_result;
    bool show_result = false;

    void execute_script();
    void save_script();
    void load_script();

public:
    ScriptEditor(RedisClient& c);
    ftxui::Component render();
};