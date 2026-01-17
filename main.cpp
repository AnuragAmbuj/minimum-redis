#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "db.h"
#include "commands.h"
#include "resp.h"

Database db;
CommandProcessor processor(db);

void handle_client(int client_fd);
void periodic_save();

const std::string DB_FILE = "minimalredis.db";

void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void handle_client(int client_fd) {
    const size_t BUFFER_SIZE = 8192;  // 8KB buffer for better performance
    std::unique_ptr<char[]> buf(new char[BUFFER_SIZE]);
    size_t buf_pos = 0;

    while (true) {
        ssize_t n = read(client_fd, buf.get() + buf_pos, BUFFER_SIZE - buf_pos - 1);
        if (n <= 0) {
            break;  // Connection closed or error
        }
        buf_pos += n;
        buf[buf_pos] = '\0';

        // Process all complete commands in the buffer
        size_t processed_pos = 0;
        while (processed_pos < buf_pos) {
            try {
                RespParser parser(buf.get() + processed_pos, buf_pos - processed_pos);
                auto command = parser.parse();

                std::string response = processor.process_command(command);
                ssize_t written = write(client_fd, response.c_str(), response.length());
                if (written < 0) {
                    return;  // Write error, close connection
                }

                // Move past the processed command
                size_t consumed = parser.get_consumed_bytes();
                processed_pos += consumed;

            } catch (const std::runtime_error& e) {
                const std::string& err_msg = e.what();
                if (err_msg == "Incomplete command") {
                    // Not enough data for a complete command, wait for more
                    break;
                } else {
                    std::string error = "-ERR " + err_msg + "\r\n";
                    write(client_fd, error.c_str(), error.length());
                    return;  // Close connection on parse error
                }
            }
        }

        // Move remaining unprocessed data to the beginning of buffer
        if (processed_pos > 0 && processed_pos < buf_pos) {
            memmove(buf.get(), buf.get() + processed_pos, buf_pos - processed_pos);
            buf_pos -= processed_pos;
        } else if (processed_pos == buf_pos) {
            buf_pos = 0;  // All data processed
        }

        // Prevent buffer overflow
        if (buf_pos >= BUFFER_SIZE - 1024) {
            // Buffer nearly full, send error and close
            const char* error = "-ERR Command too large\r\n";
            write(client_fd, error, strlen(error));
            return;
        }
    }
}

void periodic_save() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(30)); // Save every 30 seconds
        if (db.save_to_file(DB_FILE)) {
            printf("Database saved successfully\n");
        } else {
            printf("Failed to save database\n");
        }
    }
}

int main() {
    // Load database from file on startup
    if (db.load_from_file(DB_FILE)) {
        printf("Database loaded from file\n");
    } else {
        printf("No existing database file found, starting fresh\n");
    }

    // Start periodic save thread
    std::thread save_thread(periodic_save);
    save_thread.detach();

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Set SO_REUSEADDR to avoid "Address already in use" error
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(6379);
    addr.sin_addr.s_addr = ntohl(INADDR_ANY);

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    printf("MinimalRedis server listening on port 6379\n");

    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_fd < 0) {
            fprintf(stderr, "Accept failed\n");
            continue;  // Continue accepting other connections
        }

        printf("New client connection\n");
        // Handle client connection in a new thread
        std::thread client_thread([client_fd]() {
            handle_client(client_fd);
            close(client_fd);
        });
        client_thread.detach();  // Let thread run independently
        printf("Client connection closed\n");
    }
    return 0;
}