//
// Created by Sisyphus on 16/01/26.
//

#ifndef MINIMALREDIS_COMMANDS_H
#define MINIMALREDIS_COMMANDS_H

#include "resp.h"
#include "db.h"
#include <string>
#include <queue>
#include <functional>

// Transaction command representation
struct TransactionCommand {
    std::string command_name;
    std::vector<std::shared_ptr<RespValue>> args;
    std::function<std::string()> execute_func;
};

class CommandProcessor {
private:
    Database& db;
    std::queue<TransactionCommand> transaction_queue;
    bool in_transaction = false;

    std::string handle_set(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_get(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_del(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_exists(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_keys(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_save(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_type(const std::vector<std::shared_ptr<RespValue>>& args);

    // List commands
    std::string handle_lpush(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_rpush(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_lpop(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_rpop(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_llen(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_lrange(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_lindex(const std::vector<std::shared_ptr<RespValue>>& args);

    // Set commands
    std::string handle_sadd(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_srem(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_sismember(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_scard(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_smembers(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_sinter(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_sunion(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_sdiff(const std::vector<std::shared_ptr<RespValue>>& args);

    // Hash commands
    std::string handle_hset(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_hget(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_hdel(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_hlen(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_hkeys(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_hvals(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_hgetall(const std::vector<std::shared_ptr<RespValue>>& args);

    // Sorted Set commands
    std::string handle_zadd(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_zrem(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_zcard(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_zrange(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_zrevrange(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_zscore(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_zrank(const std::vector<std::shared_ptr<RespValue>>& args);

    // Transaction commands
    std::string handle_multi(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_exec(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_discard(const std::vector<std::shared_ptr<RespValue>>& args);

    // Expiration commands
    std::string handle_expire(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_pexpire(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_ttl(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_pttl(const std::vector<std::shared_ptr<RespValue>>& args);
    std::string handle_persist(const std::vector<std::shared_ptr<RespValue>>& args);

public:
    CommandProcessor(Database& database) : db(database) {}

    std::string process_command(const std::shared_ptr<RespValue>& command);
};

#endif //MINIMALREDIS_COMMANDS_H