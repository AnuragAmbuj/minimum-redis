#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

class RespParser {
private:
    std::string buffer;
    size_t pos = 0;

public:
    void append(const std::string& data);
    std::string get_next_message();
    void clear();
    bool has_data() const;
};

class RedisClient {
private:
    int sock_fd = -1;
    std::string host = "127.0.0.1";
    int port = 6379;
    std::atomic<bool> connected{false};
    RespParser parser;

    std::thread receive_thread;
    std::atomic<bool> running{false};
    std::mutex response_mutex;
    std::condition_variable response_cv;
    std::queue<std::string> response_queue;

    bool connect_to_server();
    void disconnect();

    void receive_loop();

public:
    RedisClient(const std::string& h = "127.0.0.1", int p = 6379);
    ~RedisClient();

    bool connect();
    void stop();

    bool is_connected() const { return connected; }
    std::string get_host() const { return host; }
    int get_port() const { return port; }

    bool send_command(const std::string& command);
    std::string get_response(int timeout_ms = 5000);

    bool has_pending_response() const;
    std::string get_pending_response();
};