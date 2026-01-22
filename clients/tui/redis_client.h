#pragma once

#include <string>
#include <vector>

class RespParser {
private:
    std::string buffer;
    size_t pos = 0;

public:
    void append(const std::string& data) { buffer += data; }
    std::string get_next_message();
    void clear() { buffer.clear(); pos = 0; }
    bool has_data() const { return !buffer.empty() && pos < buffer.size(); }
};

class RedisClient {
private:
    int sock_fd = -1;
    std::string host = "127.0.0.1";
    int port = 6379;
    bool connected = false;
    RespParser parser;

    bool connect_to_server();

public:
    RedisClient(const std::string& h = "127.0.0.1", int p = 6379) : host(h), port(p) {}
    ~RedisClient() { stop(); }

    bool connect() { return connected = connect_to_server(); }
    void stop();

    bool is_connected() const noexcept { return connected; }
    std::string get_host() const noexcept { return host; }
    int get_port() const noexcept { return port; }

    bool send_command(const std::string& command);
    std::string get_response(int timeout_ms = 5000);
};