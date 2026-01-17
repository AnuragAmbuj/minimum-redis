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

    // If in transaction, queue the command instead of executing it
    if (in_transaction && upper_cmd != "EXEC" && upper_cmd != "DISCARD") {
        TransactionCommand tx_cmd;
        tx_cmd.command_name = upper_cmd;
        tx_cmd.args = cmd_args;
        tx_cmd.execute_func = [this, cmd_args, upper_cmd]() -> std::string {
            // Execute the command during transaction
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
        };

        transaction_queue.push(tx_cmd);
        return "+QUEUED\r\n";
    }

    // Execute command immediately (not in transaction)
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
    }
    // Transaction commands
    else if (upper_cmd == "MULTI") {
        return handle_multi(cmd_args);
    } else if (upper_cmd == "EXEC") {
        return handle_exec(cmd_args);
    } else if (upper_cmd == "DISCARD") {
        return handle_discard(cmd_args);
    }
    // Expiration commands
    else if (upper_cmd == "EXPIRE") {
        return handle_expire(cmd_args);
    } else if (upper_cmd == "PEXPIRE") {
        return handle_pexpire(cmd_args);
    } else if (upper_cmd == "TTL") {
        return handle_ttl(cmd_args);
    } else if (upper_cmd == "PTTL") {
        return handle_pttl(cmd_args);
    } else if (upper_cmd == "PERSIST") {
        return handle_persist(cmd_args);
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

    if (!db.exists(key) || db.type(key) != RedisType::String) {
        return "$-1\r\n"; // Null bulk string for non-existent key or wrong type
    }

    std::string value = db.get(key);

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

    std::string type_str;
    if (!db.exists(key)) {
        type_str = "none";
    } else {
        RedisType type = db.type(key);
        switch (type) {
            case RedisType::String: type_str = "string"; break;
            case RedisType::List: type_str = "list"; break;
            case RedisType::Set: type_str = "set"; break;
            case RedisType::Hash: type_str = "hash"; break;
            case RedisType::ZSet: type_str = "zset"; break;
            default: type_str = "none"; break;
        }
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

// Set commands
std::string CommandProcessor::handle_sadd(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 3) {
        return "-ERR SADD requires at least 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR SADD key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> members;

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR SADD members must be bulk strings\r\n";
        }
        members.push_back(args[i]->str_val);
    }

    size_t added_count = db.sadd(key, members);
    return ":" + std::to_string(added_count) + "\r\n";
}

std::string CommandProcessor::handle_srem(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 3) {
        return "-ERR SREM requires at least 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR SREM key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> members;

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR SREM members must be bulk strings\r\n";
        }
        members.push_back(args[i]->str_val);
    }

    size_t removed_count = db.srem(key, members);
    return ":" + std::to_string(removed_count) + "\r\n";
}

std::string CommandProcessor::handle_sismember(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR SISMEMBER requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR SISMEMBER arguments must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    std::string member = args[2]->str_val;

    bool is_member = db.sismember(key, member);
    return ":" + std::to_string(is_member ? 1 : 0) + "\r\n";
}

std::string CommandProcessor::handle_scard(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR SCARD requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR SCARD key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    size_t cardinality = db.scard(key);
    return ":" + std::to_string(cardinality) + "\r\n";
}

std::string CommandProcessor::handle_smembers(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR SMEMBERS requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR SMEMBERS key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> members = db.smembers(key);

    std::string response = "*" + std::to_string(members.size()) + "\r\n";
    for (const auto& member : members) {
        response += "$" + std::to_string(member.length()) + "\r\n" + member + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_sinter(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 2) {
        return "-ERR SINTER requires at least 1 argument\r\n";
    }

    std::vector<std::string> keys;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR SINTER keys must be bulk strings\r\n";
        }
        keys.push_back(args[i]->str_val);
    }

    std::vector<std::string> intersection = db.sinter(keys);

    std::string response = "*" + std::to_string(intersection.size()) + "\r\n";
    for (const auto& member : intersection) {
        response += "$" + std::to_string(member.length()) + "\r\n" + member + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_sunion(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 2) {
        return "-ERR SUNION requires at least 1 argument\r\n";
    }

    std::vector<std::string> keys;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR SUNION keys must be bulk strings\r\n";
        }
        keys.push_back(args[i]->str_val);
    }

    std::vector<std::string> union_result = db.sunion(keys);

    std::string response = "*" + std::to_string(union_result.size()) + "\r\n";
    for (const auto& member : union_result) {
        response += "$" + std::to_string(member.length()) + "\r\n" + member + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_sdiff(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 2) {
        return "-ERR SDIFF requires at least 1 argument\r\n";
    }

    std::vector<std::string> keys;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR SDIFF keys must be bulk strings\r\n";
        }
        keys.push_back(args[i]->str_val);
    }

    std::vector<std::string> difference = db.sdiff(keys);

    std::string response = "*" + std::to_string(difference.size()) + "\r\n";
    for (const auto& member : difference) {
        response += "$" + std::to_string(member.length()) + "\r\n" + member + "\r\n";
    }

    return response;
}

// Hash commands
std::string CommandProcessor::handle_hset(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 4 || (args.size() % 2) != 0) {
        return "-ERR HSET requires an even number of arguments (at least key, field, value)\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR HSET key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    size_t fields_set = 0;

    for (size_t i = 2; i < args.size(); i += 2) {
        if (args[i]->type != RespType::BulkString || args[i + 1]->type != RespType::BulkString) {
            return "-ERR HSET field and value must be bulk strings\r\n";
        }

        std::string field = args[i]->str_val;
        std::string value = args[i + 1]->str_val;

        bool field_set = db.hset(key, field, value);
        if (field_set) {
            fields_set++;
        }
    }

    return ":" + std::to_string(fields_set) + "\r\n";
}

std::string CommandProcessor::handle_hget(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR HGET requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR HGET key and field must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    std::string field = args[2]->str_val;

    std::string value = db.hget(key, field);
    if (value.empty() && db.exists(key) && db.type(key) == RedisType::Hash) {
        // Field doesn't exist in hash
        return "$-1\r\n";
    } else if (value.empty()) {
        // Key doesn't exist
        return "$-1\r\n";
    }

    return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
}

std::string CommandProcessor::handle_hdel(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 3) {
        return "-ERR HDEL requires at least 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR HDEL key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> fields;

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR HDEL fields must be bulk strings\r\n";
        }
        fields.push_back(args[i]->str_val);
    }

    size_t fields_deleted = db.hdel(key, fields);
    return ":" + std::to_string(fields_deleted) + "\r\n";
}

std::string CommandProcessor::handle_hlen(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR HLEN requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR HLEN key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    size_t length = db.hlen(key);
    return ":" + std::to_string(length) + "\r\n";
}

std::string CommandProcessor::handle_hkeys(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR HKEYS requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR HKEYS key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> keys = db.hkeys(key);

    std::string response = "*" + std::to_string(keys.size()) + "\r\n";
    for (const auto& key_name : keys) {
        response += "$" + std::to_string(key_name.length()) + "\r\n" + key_name + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_hvals(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR HVALS requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR HVALS key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> values = db.hvals(key);

    std::string response = "*" + std::to_string(values.size()) + "\r\n";
    for (const auto& value : values) {
        response += "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_hgetall(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR HGETALL requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR HGETALL key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    auto hash = db.hgetall(key);

    std::string response = "*" + std::to_string(hash.size() * 2) + "\r\n";
    for (const auto& pair : hash) {
        // Field
        response += "$" + std::to_string(pair.first.length()) + "\r\n" + pair.first + "\r\n";
        // Value
        response += "$" + std::to_string(pair.second.length()) + "\r\n" + pair.second + "\r\n";
    }

    return response;
}

// Sorted Set commands
std::string CommandProcessor::handle_zadd(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 4 || (args.size() % 2) != 0) {
        return "-ERR ZADD requires an even number of arguments (at least key, score, member)\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR ZADD key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::pair<double, std::string>> members;

    for (size_t i = 2; i < args.size(); i += 2) {
        if (args[i]->type != RespType::BulkString || args[i + 1]->type != RespType::BulkString) {
            return "-ERR ZADD score and member must be bulk strings\r\n";
        }

        try {
            double score = std::stod(args[i]->str_val);
            std::string member = args[i + 1]->str_val;
            members.emplace_back(score, member);
        } catch (const std::exception&) {
            return "-ERR ZADD score must be a valid number\r\n";
        }
    }

    size_t members_added = db.zadd(key, members);
    return ":" + std::to_string(members_added) + "\r\n";
}

std::string CommandProcessor::handle_zrem(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 3) {
        return "-ERR ZREM requires at least 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR ZREM key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    std::vector<std::string> members;

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i]->type != RespType::BulkString) {
            return "-ERR ZREM members must be bulk strings\r\n";
        }
        members.push_back(args[i]->str_val);
    }

    size_t members_removed = db.zrem(key, members);
    return ":" + std::to_string(members_removed) + "\r\n";
}

std::string CommandProcessor::handle_zcard(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR ZCARD requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR ZCARD key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    size_t cardinality = db.zcard(key);
    return ":" + std::to_string(cardinality) + "\r\n";
}

std::string CommandProcessor::handle_zrange(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 4) {
        return "-ERR ZRANGE requires at least 3 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR ZRANGE key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;

    int start, end;
    try {
        start = std::stoi(args[2]->str_val);
        end = std::stoi(args[3]->str_val);
    } catch (const std::exception&) {
        return "-ERR ZRANGE start and end must be integers\r\n";
    }

    std::vector<std::string> range = db.zrange(key, start, end);

    std::string response = "*" + std::to_string(range.size()) + "\r\n";
    for (const auto& member : range) {
        response += "$" + std::to_string(member.length()) + "\r\n" + member + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_zrevrange(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() < 4) {
        return "-ERR ZREVRANGE requires at least 3 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR ZREVRANGE key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;

    int start, end;
    try {
        start = std::stoi(args[2]->str_val);
        end = std::stoi(args[3]->str_val);
    } catch (const std::exception&) {
        return "-ERR ZREVRANGE start and end must be integers\r\n";
    }

    std::vector<std::string> range = db.zrevrange(key, start, end);

    std::string response = "*" + std::to_string(range.size()) + "\r\n";
    for (const auto& member : range) {
        response += "$" + std::to_string(member.length()) + "\r\n" + member + "\r\n";
    }

    return response;
}

std::string CommandProcessor::handle_zscore(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR ZSCORE requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR ZSCORE key and member must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    std::string member = args[2]->str_val;

    double score = db.zscore(key, member);
    if (score == 0.0 && (!db.exists(key) || db.zcard(key) == 0)) {
        // Member doesn't exist
        return "$-1\r\n";
    }


    std::string score_str = std::to_string(score);
    size_t dot_pos = score_str.find('.');
    if (dot_pos != std::string::npos) {
        size_t last_non_zero = score_str.find_last_not_of('0');
        if (last_non_zero > dot_pos) {
            score_str = score_str.substr(0, last_non_zero + 1);
        } else {
            score_str = score_str.substr(0, dot_pos);
        }
    }

    return "$" + std::to_string(score_str.length()) + "\r\n" + score_str + "\r\n";
}

std::string CommandProcessor::handle_zrank(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR ZRANK requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR ZRANK key and member must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    std::string member = args[2]->str_val;

    long long rank = db.zrank(key, member);
    if (rank < 0) {
        return "$-1\r\n"; // Member not found
    }

    return ":" + std::to_string(rank) + "\r\n";
}



// Transaction commands
std::string CommandProcessor::handle_multi(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 1) {
        return "-ERR MULTI requires no arguments\r\n";
    }

    if (in_transaction) {
        return "-ERR MULTI cannot be nested\r\n";
    }

    in_transaction = true;
    // Clear any existing transaction queue
    while (!transaction_queue.empty()) {
        transaction_queue.pop();
    }

    return "+OK\r\n";
}

std::string CommandProcessor::handle_exec(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 1) {
        return "-ERR EXEC requires no arguments\r\n";
    }

    if (!in_transaction) {
        return "-ERR EXEC without MULTI\r\n";
    }

    // Execute all queued commands atomically
    std::vector<std::string> results;
    results.reserve(transaction_queue.size());

    while (!transaction_queue.empty()) {
        auto& cmd = transaction_queue.front();
        try {
            std::string result = cmd.execute_func();
            results.push_back(result);
        } catch (const std::exception& e) {
            // On error, we still continue with other commands in Redis
            results.push_back("-ERR " + std::string(e.what()) + "\r\n");
        }
        transaction_queue.pop();
    }

    in_transaction = false;

    // Format response as array of results
    std::string response = "*" + std::to_string(results.size()) + "\r\n";
    for (const auto& result : results) {
        response += result;
    }

    return response;
}

std::string CommandProcessor::handle_discard(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 1) {
        return "-ERR DISCARD requires no arguments\r\n";
    }

    if (!in_transaction) {
        return "-ERR DISCARD without MULTI\r\n";
    }

    // Clear transaction queue
    while (!transaction_queue.empty()) {
        transaction_queue.pop();
    }

    in_transaction = false;
    return "+OK\r\n";
}

// Expiration commands
std::string CommandProcessor::handle_expire(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR EXPIRE requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR EXPIRE arguments must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    int64_t seconds;

    try {
        seconds = std::stoll(args[2]->str_val);
    } catch (const std::exception&) {
        return "-ERR EXPIRE seconds must be an integer\r\n";
    }

    bool result = db.expire(key, seconds);
    return result ? ":1\r\n" : ":0\r\n";
}

std::string CommandProcessor::handle_pexpire(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 3) {
        return "-ERR PEXPIRE requires exactly 2 arguments\r\n";
    }

    if (args[1]->type != RespType::BulkString || args[2]->type != RespType::BulkString) {
        return "-ERR PEXPIRE arguments must be bulk strings\r\n";
    }

    std::string key = args[1]->str_val;
    int64_t milliseconds;

    try {
        milliseconds = std::stoll(args[2]->str_val);
    } catch (const std::exception&) {
        return "-ERR PEXPIRE milliseconds must be an integer\r\n";
    }

    bool result = db.pexpire(key, milliseconds);
    return result ? ":1\r\n" : ":0\r\n";
}

std::string CommandProcessor::handle_ttl(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR TTL requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR TTL key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    int64_t ttl = db.ttl(key);

    return ":" + std::to_string(ttl) + "\r\n";
}

std::string CommandProcessor::handle_pttl(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR PTTL requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR PTTL key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    int64_t pttl = db.pttl(key);

    return ":" + std::to_string(pttl) + "\r\n";
}

std::string CommandProcessor::handle_persist(const std::vector<std::shared_ptr<RespValue>>& args) {
    if (args.size() != 2) {
        return "-ERR PERSIST requires exactly 1 argument\r\n";
    }

    if (args[1]->type != RespType::BulkString) {
        return "-ERR PERSIST key must be a bulk string\r\n";
    }

    std::string key = args[1]->str_val;
    bool result = db.persist(key);

    return result ? ":1\r\n" : ":0\r\n";
}