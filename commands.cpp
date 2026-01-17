//
// Created by Sisyphus on 16/01/26.
//

#include "commands.h"
#include <algorithm>

std::string CommandProcessor::process_command(const std::shared_ptr<RespValue>& command) {
    if (command->type != RespType::Array || command->array_val.empty()) {
        return "-ERR Invalid command format\r\n";
    }

    auto& cmd_args = command->array_val;
    if (cmd_args[0]->type != RespType::BulkString) {
        return "-ERR Command name must be a bulk string\r\n";
    }

    const std::string& cmd_name = cmd_args[0]->str_val;

    // Convert to uppercase for case-insensitive comparison
    std::string upper_cmd;
    upper_cmd.reserve(cmd_name.size());
    for (char c : cmd_name) {
        upper_cmd += ::toupper(c);
    }

    if (upper_cmd == "SET") {
        return handle_set(cmd_args);
    } else if (upper_cmd == "GET") {
        return handle_get(cmd_args);
    } else if (upper_cmd == "DEL") {
        return handle_del(cmd_args);
    } else if (upper_cmd == "EXISTS") {
        return handle_exists(cmd_args);
    } else if (upper_cmd == "KEYS") {
        return handle_keys(cmd_args);
    } else if (upper_cmd == "SAVE") {
        return handle_save(cmd_args);
    } else if (upper_cmd == "TYPE") {
        return handle_type(cmd_args);
    }
    // List commands
    else if (upper_cmd == "LPUSH") {
        return handle_lpush(cmd_args);
    } else if (upper_cmd == "RPUSH") {
        return handle_rpush(cmd_args);
    } else if (upper_cmd == "LPOP") {
        return handle_lpop(cmd_args);
    } else if (upper_cmd == "RPOP") {
        return handle_rpop(cmd_args);
    } else if (upper_cmd == "LLEN") {
        return handle_llen(cmd_args);
    } else if (upper_cmd == "LRANGE") {
        return handle_lrange(cmd_args);
    } else if (upper_cmd == "LINDEX") {
        return handle_lindex(cmd_args);
    }
    // Set commands
    else if (upper_cmd == "SADD") {
        return handle_sadd(cmd_args);
    } else if (upper_cmd == "SREM") {
        return handle_srem(cmd_args);
    } else if (upper_cmd == "SISMEMBER") {
        return handle_sismember(cmd_args);
    } else if (upper_cmd == "SCARD") {
        return handle_scard(cmd_args);
    } else if (upper_cmd == "SMEMBERS") {
        return handle_smembers(cmd_args);
    } else if (upper_cmd == "SINTER") {
        return handle_sinter(cmd_args);
    } else if (upper_cmd == "SUNION") {
        return handle_sunion(cmd_args);
    } else if (upper_cmd == "SDIFF") {
        return handle_sdiff(cmd_args);
    }
    // Hash commands
    else if (upper_cmd == "HSET") {
        return handle_hset(cmd_args);
    } else if (upper_cmd == "HGET") {
        return handle_hget(cmd_args);
    } else if (upper_cmd == "HDEL") {
        return handle_hdel(cmd_args);
    } else if (upper_cmd == "HLEN") {
        return handle_hlen(cmd_args);
    } else if (upper_cmd == "HKEYS") {
        return handle_hkeys(cmd_args);
    } else if (upper_cmd == "HVALS") {
        return handle_hvals(cmd_args);
    } else if (upper_cmd == "HGETALL") {
        return handle_hgetall(cmd_args);
    }
    // Sorted Set commands
    else if (upper_cmd == "ZADD") {
        return handle_zadd(cmd_args);
    } else if (upper_cmd == "ZREM") {
        return handle_zrem(cmd_args);
    } else if (upper_cmd == "ZCARD") {
        return handle_zcard(cmd_args);
    } else if (upper_cmd == "ZRANGE") {
        return handle_zrange(cmd_args);
    } else if (upper_cmd == "ZREVRANGE") {
        return handle_zrevrange(cmd_args);
    } else if (upper_cmd == "ZSCORE") {
        return handle_zscore(cmd_args);
    } else if (upper_cmd == "ZRANK") {
        return handle_zrank(cmd_args);
    } else {
        return "-ERR Unknown command\r\n";
    }
}

std::string CommandProcessor::handle_set(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR SET requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR SET arguments must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    std::string value = args[2]->str_val;

    if (db.set(key, value)) {
        return "+OK\r\n";
    } else {
        return "-ERR Failed to set value\r\n";
    }
}

std::string CommandProcessor::handle_get(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR GET requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR GET argument must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::string value = db.get(key);

    if (value.empty() && !db.exists(key)) {
        return "$-1\r\n"; // Null bulk string for non-existent key
    }

    return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
}

std::string CommandProcessor::handle_del(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR DEL requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR DEL argument must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    int deleted = db.del(key) ? 1 : 0;

    return ":" + std::to_string(deleted) + "\r\n";
}

std::string CommandProcessor::handle_exists(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR EXISTS requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR EXISTS argument must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    int exists = db.exists(key) ? 1 : 0;

    return ":" + std::to_string(exists) + "\r\n";
}

std::string CommandProcessor::handle_keys(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 1) {
        return "-ERR KEYS requires no arguments\r\n";
    }

    std::vector<std::string> keys = db.keys();

    std::string response = "*" + std::to_string(keys.size()) + "\r\n";
    for (const std::string& key : keys) {
        response += "$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_save(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 1) {
        return "-ERR SAVE requires no arguments\r\n";
    }

    const std::string filename = "minimalredis.db";
    if (db.save_to_file(filename)) {
        return "+OK\r\n";
    } else {
        return "-ERR Failed to save database\r\n";
    }
}

std::string CommandProcessor::handle_type(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR TYPE requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR TYPE argument must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    RedisType type = db.type(key);

    std::string type_str;
    switch (type) {
        case RedisType::String: type_str = "string"; break;
        case RedisType::List: type_str = "list"; break;
        case RedisType::Set: type_str = "set"; break;
        case RedisType::Hash: type_str = "hash"; break;
        case RedisType::ZSet: type_str = "zset"; break;
        default: type_str = "none"; break;
    }

    return "+" + type_str + "\r\n";
}

// List commands
std::string CommandProcessor::handle_lpush(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 3) {
        return "-ERR LPUSH requires at least 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR LPUSH key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> values;

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR LPUSH values must be bulk strings\r\n";
        }
        values.push_back(args[i]->str_val);
    }

    size_t new_length = db.lpush(key, values);
    return ":" + std::to_string(new_length) + "\r\n";
}

std::string CommandProcessor::handle_rpush(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 3) {
        return "-ERR RPUSH requires at least 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR RPUSH key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> values;

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR RPUSH values must be bulk strings\r\n";
        }
        values.push_back(args[i]->str_val);
    }

    size_t new_length = db.rpush(key, values);
    return ":" + std::to_string(new_length) + "\r\n";
}

std::string CommandProcessor::handle_lpop(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR LPOP requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR LPOP key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::string value = db.lpop(key);

    if (value.empty() && !db.exists(key)) {
        return "$-1\r\n"; // Null bulk string
    }

    return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
}

std::string CommandProcessor::handle_rpop(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR RPOP requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR RPOP key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::string value = db.rpop(key);

    if (value.empty() && !db.exists(key)) {
        return "$-1\r\n"; // Null bulk string
    }

    return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
}

std::string CommandProcessor::handle_llen(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR LLEN requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR LLEN key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    size_t length = db.llen(key);

    return ":" + std::to_string(length) + "\r\n";
}

std::string CommandProcessor::handle_lrange(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 4) {
        return "-ERR LRANGE requires exactly 3 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR LRANGE key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    int start, end;

    try {
        start = std::stoi(args[2]->str_val);
        end = std::stoi(args[3]->str_val);
    } catch (const std::exception&) {
        return "-ERR LRANGE start and end must be integers\r\n";
    }

    std::vector<std::string> range = db.lrange(key, start, end);

    std::string response = "*" + std::to_string(range.size()) + "\r\n";
    for (const std::string& item : range) {
        response += "$" + std::to_string(item.length()) + "\r\n" + item + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_lindex(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR LINDEX requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR LINDEX key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    int index;

    try {
        index = std::stoi(args[2]->str_val);
    } catch (const std::exception&) {
        return "-ERR LINDEX index must be an integer\r\n";
    }

    std::string value = db.lindex(key, index);

    if (value.empty() && db.exists(key)) {
        return "$-1\r\n"; // Null bulk string for out of range
    } else if (value.empty()) {
        return "$-1\r\n"; // Key doesn't exist
    }

    return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
}

// Set commands - stub implementations
std::string CommandProcessor::handle_sadd(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_srem(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_sismember(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_scard(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_smembers(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_sinter(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_sunion(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_sdiff(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

// Hash commands - stub implementations
std::string CommandProcessor::handle_hset(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_hget(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_hdel(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_hlen(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_hkeys(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_hvals(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_hgetall(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

// Sorted Set commands - stub implementations
std::string CommandProcessor::handle_zadd(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_zrem(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_zcard(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_zrange(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_zrevrange(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_zscore(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}

std::string CommandProcessor::handle_zrank(const std::vector<std::shared_ptr<RespValue>>& args) {
    return "-ERR Not implemented yet\r\n";
}