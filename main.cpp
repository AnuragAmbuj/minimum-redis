#include "db.h"
#include "commands.h"
#include "resp.h"
#include "cluster.h"
#include "save_daemon.h"
#include <iostream>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

Database db;
std::unordered_map<int, CommandProcessor*> client_processors;

// SaveDaemon instance for periodic saves
SaveDaemon* save_daemon = nullptr;

constexpr int DEFAULT_PORT = 6379;
constexpr size_t BUFFER_SIZE = 8192;  // 8KB buffer for better performance

void handle_client(int client_fd);
void periodic_save();

const std::string DB_FILE = "minimalredis.rdb";

void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void handle_client(int client_fd) {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        return;
    }
    int notify_read_fd = pipe_fds[0];
    int notify_write_fd = pipe_fds[1];

    CommandProcessor processor(db, client_fd, notify_write_fd, &client_processors, save_daemon);
    client_processors[client_fd] = &processor;

    // Use constexpr BUFFER_SIZE defined at top of file
    std::unique_ptr<char[]> buf(new char[BUFFER_SIZE]);
    size_t buf_pos = 0;

    fd_set read_fds;
    int max_fd = std::max(client_fd, notify_read_fd) + 1;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        FD_SET(notify_read_fd, &read_fds);

        int ready = select(max_fd, &read_fds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            break;
        }

        if (FD_ISSET(notify_read_fd, &read_fds)) {
            char dummy;
            read(notify_read_fd, &dummy, 1);

            while (processor.has_pubsub_messages()) {
                std::string msg = processor.get_next_pubsub_message();
                ssize_t written = write(client_fd, msg.c_str(), msg.length());
                if (written < 0) {
                    goto cleanup;
                }
            }
        }
        if (FD_ISSET(client_fd, &read_fds)) {
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
                        goto cleanup;
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
                        goto cleanup;
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
                goto cleanup;
            }
        }
    }

cleanup:
    client_processors.erase(client_fd);
    close(notify_read_fd);
    close(notify_write_fd);
}

auto main() -> int {
    // Initialize cluster configuration for MVC demo
    initialize_cluster_mvc();

    // Load database from file on startup
    if (db.load_rdb(DB_FILE)) {
        printf("Database loaded from RDB file\n");
    } else {
        printf("No existing RDB file found, starting fresh\n");
    }

    // Initialize and start save daemon with event publishing
    save_daemon = new SaveDaemon(DB_FILE,
        [](const std::string& file) {
            return db.save_rdb(file);
        },
        [](const std::string& channel, const std::string& message) {
            // Publish save events via Pub/Sub (similar to PUBLISH command)
            auto subscribers = db.get_pubsub().get_subscribers(channel);
            std::string channel_len = std::to_string(channel.length());
            std::string message_len = std::to_string(message.length());
            std::string pubsub_msg = "*3\r\n$7\r\nmessage\r\n$" + channel_len + "\r\n" + channel + "\r\n$" + message_len + "\r\n" + message + "\r\n";

            // Send to all subscribers (simplified - no client_processors_ptr check for now)
            for (int sub_fd : subscribers) {
                // In a real implementation, we'd need access to client_processors to queue messages
                // For now, just log the event
                std::cout << "[SaveEvent] " << channel << ": " << message << " sent to client " << sub_fd << std::endl;
            }
        });
    save_daemon->start();
    printf("Save daemon started (interval: %lld seconds)\n", save_daemon->get_interval().count());

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Set SO_REUSEADDR to avoid "Address already in use" error
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(DEFAULT_PORT);
    addr.sin_addr.s_addr = ntohl(INADDR_ANY);

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

        printf("MinimalRedis server listening on port %d\n", DEFAULT_PORT);

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
        client_thread.detach(); // Let thread run independently
        printf("Client connection closed\n");
    }

    // Cleanup save daemon
    if (save_daemon) {
        printf("Stopping save daemon...\n");
        save_daemon->stop();
        delete save_daemon;
        printf("Save daemon stopped\n");
    }

    return 0;
}