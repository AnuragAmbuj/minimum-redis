#include "redis_client.h"
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

bool RedisClient::connect_to_server() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) return false;

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        close(sock_fd);
        return false;
    }

    if (::connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock_fd);
        return false;
    }

    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

    return true;
}

void RedisClient::stop() {
    running = false;
    if (receive_thread.joinable()) {
        receive_thread.join();
    }
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
    connected = false;
}

bool RedisClient::send_command(const std::string& command) {
    if (!connected) return false;

    std::vector<std::string> parts;
    std::stringstream ss(command);
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }

    if (parts.empty()) return false;

    std::stringstream resp;
    resp << "*" << parts.size() << "\r\n";
    for (const auto& p : parts) {
        resp << "$" << p.length() << "\r\n" << p << "\r\n";
    }

    std::string cmd = resp.str();
    ssize_t sent = send(sock_fd, cmd.c_str(), cmd.length(), 0);
    return sent == static_cast<ssize_t>(cmd.length());
}

std::string RedisClient::get_response(int timeout_ms) {
    if (!connected) return "";

    fd_set read_fds;
    struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);

    int ready = select(sock_fd + 1, &read_fds, nullptr, nullptr, &tv);
    if (ready <= 0) return "";

    char buffer[8192];
    ssize_t n = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        connected = false;
        return "";
    }

    buffer[n] = '\0';
    parser.append(buffer);

    std::string msg = parser.get_next_message();
    return msg;
}

std::string RespParser::get_next_message() {
    if (buffer.empty() || pos >= buffer.size()) return "";

    size_t end_pos = buffer.find("\r\n", pos);
    if (end_pos == std::string::npos) return "";

    std::string message = buffer.substr(pos, end_pos - pos + 2);
    pos = end_pos + 2;
    return message;
}