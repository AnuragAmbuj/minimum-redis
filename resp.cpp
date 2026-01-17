//
// Created by Sisyphus on 16/01/26.
//

#include "resp.h"
#include <stdexcept>
#include <iostream>

std::shared_ptr<RespValue> RespParser::parse() {
    if (pos >= len) {
        throw std::runtime_error("Incomplete command");
    }

    char type_char = buf[pos++];
    switch (type_char) {
        case '+':
            return parse_simple_string();
        case '-':
            return parse_error();
        case ':':
            return parse_integer();
        case '$':
            return parse_bulk_string();
        case '*':
            return parse_array();
        default:
            throw std::runtime_error("Unknown RESP type");
    }
}

std::shared_ptr<RespValue> RespParser::parse_simple_string() {
    auto val = std::make_shared<RespValue>(RespType::SimpleString);
    val->str_val = read_line();
    if (!val->str_val.empty() && val->str_val.back() == '\r') {
        val->str_val.pop_back();
    }
    return val;
}

std::shared_ptr<RespValue> RespParser::parse_error() {
    auto val = std::make_shared<RespValue>(RespType::Error);
    val->str_val = read_line();
    if (!val->str_val.empty() && val->str_val.back() == '\r') {
        val->str_val.pop_back();
    }
    return val;
}

std::shared_ptr<RespValue> RespParser::parse_integer() {
    auto val = std::make_shared<RespValue>(RespType::Integer);
    std::string line = read_line();
    try {
        val->int_val = std::stoll(line);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid integer format");
    }
    return val;
}

std::shared_ptr<RespValue> RespParser::parse_bulk_string() {
    std::string length_str = read_line();

    // Handle null bulk string
    if (length_str == "-1") {
        auto val = std::make_shared<RespValue>(RespType::BulkString);
        val->str_val = "";  // Null bulk string represented as empty string
        return val;
    }

    size_t length;
    try {
        length = std::stoul(length_str);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid bulk string length");
    }

    auto val = std::make_shared<RespValue>(RespType::BulkString);
    val->str_val = read_bulk_string(length);
    return val;
}

std::shared_ptr<RespValue> RespParser::parse_array() {
    std::string count_str = read_line();
    size_t count;
    try {
        count = std::stoul(count_str);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid array length");
    }

    auto val = std::make_shared<RespValue>(RespType::Array);
    val->array_val.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        val->array_val.push_back(parse());
    }

    return val;
}

std::string RespParser::read_line() {
    size_t start = pos;
    while (pos < len && buf[pos] != '\n') {
        ++pos;
    }
    if (pos >= len) {
        throw std::runtime_error("Incomplete line");
    }
    size_t line_len = pos - start - 1; // exclude \r
    ++pos; // skip \n
    return std::string(buf + start, line_len);
}

std::string RespParser::read_bulk_string(size_t length) {
    if (pos + length + 2 > len) {
        throw std::runtime_error("Incomplete bulk string");
    }
    std::string str(buf + pos, length);
    pos += length + 2;
    return str;
}