#include "db.h"
#include "commands.h"
#include "resp.h"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <unordered_map>
#include <getopt.h>

Database db;
std::unordered_map<int, CommandProcessor*> client_processors;

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

    CommandProcessor processor(db, client_fd, notify_write_fd, &client_processors);
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

void periodic_save() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(30)); // Save every 30 seconds
        if (db.save_rdb(DB_FILE)) {
            printf("Database saved to RDB successfully\n");
        } else {
            printf("Failed to save database to RDB\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    bool cluster_enabled = false;
    std::string node_id;

    // Parse command line arguments
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"cluster-enabled", no_argument, 0, 'c'},
        {"node-id", required_argument, 0, 'n'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:cn:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                break;
            case 'c':
                cluster_enabled = true;
                break;
            case 'n':
                node_id = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [--port PORT] [--cluster-enabled] [--node-id ID]\n", argv[0]);
                return 1;
        }
    }

    printf("Starting MinimalRedis server on port %d", port);
    if (cluster_enabled) {
        printf(" (cluster mode enabled");
        if (!node_id.empty()) {
            printf(", node ID: %s", node_id.c_str());
        }
        printf(")");
    }
    printf("\n");

    // Load database from file on startup
    if (db.load_rdb(DB_FILE)) {
        printf("Database loaded from RDB file\n");
    } else {
        printf("No existing RDB file found, starting fresh\n");
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
    addr.sin_port = ntohs(port);
    addr.sin_addr.s_addr = ntohl(INADDR_ANY);

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

        printf("MinimalRedis server listening on port %d\n", port);

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