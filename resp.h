//
// Created by Sisyphus on 16/01/26.
//

#ifndef MINIMALREDIS_RESP_H
#define MINIMALREDIS_RESP_H

#include <vector>
#include <string>
#include <memory>

// RESP Value types
enum class RespType {
    SimpleString,
    Error,
    Integer,
    BulkString,
    Array
};

// RESP Value class
class RespValue {
public:
    RespType type;
    std::string str_val;
    long long int_val;
    std::vector<std::shared_ptr<RespValue>> array_val;

    RespValue(RespType t) : type(t), int_val(0) {}
};

// RESP Parser class
class RespParser {
private:
    const char* buf;
    size_t len;
    size_t pos;

public:
    RespParser(const char* buffer, size_t length) : buf(buffer), len(length), pos(0) {}

    std::shared_ptr<RespValue> parse();
    size_t get_consumed_bytes() const noexcept { return pos; }

private:
    std::shared_ptr<RespValue> parse_simple_string();
    std::shared_ptr<RespValue> parse_error();
    std::shared_ptr<RespValue> parse_integer();
    std::shared_ptr<RespValue> parse_bulk_string();
    std::shared_ptr<RespValue> parse_array();

    std::string read_line();
    std::string read_bulk_string(size_t length);
};

#endif //MINIMALREDIS_RESP_H