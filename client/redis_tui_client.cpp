#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ncurses.h>
#include <panel.h>
#include <form.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>

// Redis protocol parsing (simplified)
class RespParser {
private:
    std::string buffer;
    size_t pos = 0;

public:
    void append(const std::string& data) {
        buffer += data;
    }

    std::string get_next_message() {
        if (buffer.empty() || pos >= buffer.size()) return "";

        size_t end_pos = buffer.find("\r\n", pos);
        if (end_pos == std::string::npos) return "";

        std::string message = buffer.substr(pos, end_pos - pos + 2);
        pos = end_pos + 2;
        return message;
    }

    void clear() {
        buffer.clear();
        pos = 0;
    }

    bool has_data() const {
        return !buffer.empty() && pos < buffer.size();
    }
};

class RedisClient {
private:
    int sock_fd = -1;
    std::string host = "127.0.0.1";
    int port = 6379;
    bool connected = false;
    RespParser parser;

    // Threading
    std::thread receive_thread;
    std::atomic<bool> running{false};
    std::mutex response_mutex;
    std::condition_variable response_cv;
    std::queue<std::string> response_queue;

    // Connection management
    bool connect_to_redis() {
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

        // Set non-blocking
        int flags = fcntl(sock_fd, F_GETFL, 0);
        fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

        connected = true;
        return true;
    }

    void disconnect() {
        connected = false;
        if (sock_fd >= 0) {
            close(sock_fd);
            sock_fd = -1;
        }
    }

    void receive_loop() {
        char buffer[8192];
        while (running && connected) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock_fd, &read_fds);

            struct timeval tv = {0, 100000}; // 100ms timeout
            int ready = select(sock_fd + 1, &read_fds, nullptr, nullptr, &tv);

            if (ready > 0 && FD_ISSET(sock_fd, &read_fds)) {
                ssize_t n = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    buffer[n] = '\0';
                    parser.append(buffer);

                    // Process all available messages
                    std::string msg;
                    while (!(msg = parser.get_next_message()).empty()) {
                        std::lock_guard<std::mutex> lock(response_mutex);
                        response_queue.push(msg);
                        response_cv.notify_one();
                    }
                } else if (n == 0) {
                    // Connection closed
                    connected = false;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

public:
    RedisClient(const std::string& h = "127.0.0.1", int p = 6379) : host(h), port(p) {}

    ~RedisClient() {
        stop();
    }

    bool connect_to_server() {
        if (connected) return true;

        if (!connect_to_redis()) return false;

        running = true;
        receive_thread = std::thread(&RedisClient::receive_loop, this);
        return true;
    }

    void stop() {
        running = false;
        if (receive_thread.joinable()) {
            receive_thread.join();
        }
        disconnect();
    }

    bool is_connected() const { return connected; }

    bool send_command(const std::string& command) {
        if (!connected) return false;

        // Format as RESP array
        std::stringstream ss;
        std::vector<std::string> parts;
        std::stringstream cmd_stream(command);
        std::string part;
        while (std::getline(cmd_stream, part, ' ')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        if (parts.empty()) return false;

        ss << "*" << parts.size() << "\r\n";
        for (const auto& p : parts) {
            ss << "$" << p.length() << "\r\n" << p << "\r\n";
        }

        std::string resp_command = ss.str();
        ssize_t sent = send(sock_fd, resp_command.c_str(), resp_command.length(), 0);
        return sent == static_cast<ssize_t>(resp_command.length());
    }

    std::string get_response(int timeout_ms = 5000) {
        std::unique_lock<std::mutex> lock(response_mutex);
        if (response_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                [this]() { return !response_queue.empty(); })) {
            std::string response = response_queue.front();
            response_queue.pop();
            return response;
        }
        return "";
    }

    std::string get_host() const { return host; }
    int get_port() const { return port; }
};

class TUIClient {
private:
    RedisClient client;
    WINDOW* main_win;
    WINDOW* input_win;
    WINDOW* output_win;
    WINDOW* status_win;
    PANEL* main_panel;
    PANEL* input_panel;
    PANEL* output_panel;
    PANEL* status_panel;

    std::vector<std::string> command_history;
    std::vector<std::string> output_history;
    size_t history_index = 0;
    std::string current_input;
    size_t cursor_pos = 0;
    std::vector<std::string> completion_candidates;
    size_t completion_index = 0;
    std::string completion_prefix;

    bool pubsub_mode = false;
    bool running = true;

    std::vector<std::string> redis_commands = {
        "SET", "GET", "DEL", "EXISTS", "KEYS", "TYPE", "RENAME", "RENAMENX",
        "LPUSH", "RPUSH", "LPOP", "RPOP", "LLEN", "LRANGE", "LINDEX", "LSET", "LTRIM",
        "SADD", "SREM", "SISMEMBER", "SCARD", "SMEMBERS", "SUNION", "SINTER", "SDIFF",
        "HSET", "HGET", "HDEL", "HLEN", "HKEYS", "HVALS", "HGETALL", "HEXISTS",
        "ZADD", "ZREM", "ZCARD", "ZRANGE", "ZREVRANGE", "ZSCORE", "ZRANK",
        "MULTI", "EXEC", "DISCARD", "WATCH", "UNWATCH",
        "EXPIRE", "PEXPIRE", "TTL", "PTTL", "PERSIST", "EXPIREAT", "PEXPIREAT",
        "SUBSCRIBE", "PUBLISH", "UNSUBSCRIBE", "PSUBSCRIBE", "PUNSUBSCRIBE",
        "EVAL", "EVALSHA", "SCRIPT",
        "SAVE", "BGSAVE", "LASTSAVE", "FLUSHDB", "FLUSHALL", "DBSIZE",
        "INFO", "CLIENT", "CONFIG", "DEBUG", "COMMAND", "MEMORY"
    };

    void update_completion_candidates();
    void perform_tab_completion();
    void save_history_to_file();
    void load_history_from_file();

    void init_curses() {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(1);
        start_color();
        use_default_colors();

        init_pair(1, COLOR_GREEN, -1);    // Status connected
        init_pair(2, COLOR_RED, -1);      // Status disconnected
        init_pair(3, COLOR_YELLOW, -1);   // Commands
        init_pair(4, COLOR_CYAN, -1);     // Responses
        init_pair(5, COLOR_MAGENTA, -1);  // PubSub messages
    }

    void create_windows() {
        int height, width;
        getmaxyx(stdscr, height, width);

        // Status bar (top)
        status_win = newwin(1, width, 0, 0);
        status_panel = new_panel(status_win);

        // Output window (middle, most of screen)
        output_win = newwin(height - 4, width, 1, 0);
        output_panel = new_panel(output_win);
        scrollok(output_win, TRUE);

        // Input window (bottom)
        input_win = newwin(3, width, height - 3, 0);
        input_panel = new_panel(input_win);

        // Main window (for borders, etc.)
        main_win = newwin(height, width, 0, 0);
        main_panel = new_panel(main_win);

        // Draw borders
        box(main_win, 0, 0);
        mvwprintw(main_win, 0, 2, " MinimalRedis TUI Client ");
        wrefresh(main_win);
    }

    void update_status() {
        werase(status_win);
        if (client.is_connected()) {
            wattron(status_win, COLOR_PAIR(1));
            mvwprintw(status_win, 0, 0, "Connected to %s:%d",
                     client.get_host().c_str(), client.get_port());
            wattroff(status_win, COLOR_PAIR(1));
        } else {
            wattron(status_win, COLOR_PAIR(2));
            mvwprintw(status_win, 0, 0, "Disconnected - Press F2 to reconnect");
            wattroff(status_win, COLOR_PAIR(2));
        }

        if (pubsub_mode) {
            wattron(status_win, COLOR_PAIR(5));
            wprintw(status_win, " [PUBSUB MODE]");
            wattroff(status_win, COLOR_PAIR(5));
        }

        wprintw(status_win, " | History: %zu | F1: Help | F2: Connect/Disconnect | F3: Clear | TAB: Complete | F10: Quit",
               command_history.size());

        if (!completion_candidates.empty()) {
            wprintw(status_win, " | Complete: %s (%zu/%zu)",
                   completion_candidates[completion_index % completion_candidates.size()].c_str(),
                   (completion_index % completion_candidates.size()) + 1,
                   completion_candidates.size());
        }
        wrefresh(status_win);
    }

    void update_output() {
        werase(output_win);

        // Display recent output history
        int line = 0;
        int max_lines = getmaxy(output_win) - 1;

        for (auto it = output_history.rbegin();
             it != output_history.rend() && line < max_lines; ++it, ++line) {
            if (it->find("*3\r\n$7\r\nmessage\r\n") == 0) {
                // PubSub message
                wattron(output_win, COLOR_PAIR(5));
                wprintw(output_win, "[PUBSUB] ");
                wattroff(output_win, COLOR_PAIR(5));
            } else if (it->find("$") == 0 || it->find("*") == 0 || it->find("+") == 0 ||
                      it->find("-") == 0 || it->find(":") == 0) {
                // Redis response
                wattron(output_win, COLOR_PAIR(4));
                wprintw(output_win, "[RESP] ");
                wattroff(output_win, COLOR_PAIR(4));
            }

            // Simple display - in production, you'd parse RESP properly
            std::string display = *it;
            // Remove trailing \r\n for display
            if (display.size() >= 2 && display.substr(display.size() - 2) == "\r\n") {
                display = display.substr(0, display.size() - 2);
            }

            wprintw(output_win, "%s\n", display.c_str());
        }

        wrefresh(output_win);
    }

    void update_input() {
        werase(input_win);
        box(input_win, 0, 0);

        mvwprintw(input_win, 0, 2, " Command Input ");
        mvwprintw(input_win, 1, 2, "> %s", current_input.c_str());

        // Show cursor
        if (cursor_pos < current_input.length()) {
            mvwchgat(input_win, 1, 4 + cursor_pos, 1, A_REVERSE, 0, NULL);
        } else {
            mvwchgat(input_win, 1, 4 + current_input.length(), 1, A_REVERSE, 0, NULL);
        }

        wrefresh(input_win);
    }

    void add_to_output(const std::string& text) {
        output_history.push_back(text);
        // Keep only last 1000 lines
        if (output_history.size() > 1000) {
            output_history.erase(output_history.begin());
        }
        update_output();
    }

    void execute_command(const std::string& cmd) {
        if (cmd.empty()) return;

        command_history.push_back(cmd);
        history_index = command_history.size();

        add_to_output("> " + cmd);

        if (!client.is_connected()) {
            add_to_output("Not connected to server");
            return;
        }

        if (!client.send_command(cmd)) {
            add_to_output("Failed to send command");
            return;
        }

        // Special handling for SUBSCRIBE
        if (cmd.find("SUBSCRIBE") == 0) {
            pubsub_mode = true;
            add_to_output("Entered PUBSUB mode - waiting for messages...");
        } else if (cmd.find("UNSUBSCRIBE") == 0 && cmd.find(" ") == std::string::npos) {
            pubsub_mode = false;
            add_to_output("Exited PUBSUB mode");
        }

        // Get response
        std::string response = client.get_response();
        if (!response.empty()) {
            add_to_output(response);
        } else {
            add_to_output("(timeout or no response)");
        }
    }

    void handle_input(int ch) {
        switch (ch) {
            case KEY_BACKSPACE:
            case 127: // Delete key
                if (cursor_pos > 0) {
                    current_input.erase(cursor_pos - 1, 1);
                    cursor_pos--;
                }
                break;

            case KEY_DC: // Delete key
                if (cursor_pos < current_input.length()) {
                    current_input.erase(cursor_pos, 1);
                }
                break;

            case KEY_LEFT:
                if (cursor_pos > 0) cursor_pos--;
                break;

            case KEY_RIGHT:
                if (cursor_pos < current_input.length()) cursor_pos++;
                break;

            case KEY_UP:
                if (!command_history.empty()) {
                    if (history_index > 0) history_index--;
                    current_input = command_history[history_index];
                    cursor_pos = current_input.length();
                }
                break;

            case KEY_DOWN:
                if (history_index < command_history.size() - 1) {
                    history_index++;
                    current_input = command_history[history_index];
                } else {
                    history_index = command_history.size();
                    current_input.clear();
                }
                cursor_pos = current_input.length();
                break;

            case KEY_HOME:
                cursor_pos = 0;
                break;

            case KEY_END:
                cursor_pos = current_input.length();
                break;

            case '\n':
            case KEY_ENTER:
                execute_command(current_input);
                current_input.clear();
                cursor_pos = 0;
                break;

            case KEY_F(1): // Help
                add_to_output("=== MinimalRedis TUI Client Help ===");
                add_to_output("F1: Help | F2: Connect/Disconnect | F3: Clear | F10: Quit");
                add_to_output("TAB: Auto-complete commands | Arrows: Navigate history");
                add_to_output("Enter: Execute command | Backspace/Delete: Edit input");
                add_to_output("Available commands: SET, GET, DEL, KEYS, LPUSH, etc.");
                break;

            case KEY_F(2): // Connect/Disconnect
                if (client.is_connected()) {
                    client.stop();
                    pubsub_mode = false;
                    add_to_output("Disconnected from server");
                } else {
        if (client.connect_to_server()) {
            add_to_output("Connected to server");
        } else {
            add_to_output("Failed to connect to server");
        }
                }
                break;

            case KEY_F(3): // Clear output
                output_history.clear();
                update_output();
                break;

            case KEY_F(10): // Quit
                running = false;
                break;

            case '\t':
            case KEY_STAB:
                perform_tab_completion();
                break;

            default:
                if (ch >= 32 && ch <= 126) { // Printable characters
                    current_input.insert(cursor_pos, 1, ch);
                    cursor_pos++;
                }
                break;
        }
    }

public:
    TUIClient(const std::string& host = "127.0.0.1", int port = 6379)
        : client(host, port) {}

    ~TUIClient() {
        save_history_to_file();
        client.stop();
        if (main_win) delwin(main_win);
        if (input_win) delwin(input_win);
        if (output_win) delwin(output_win);
        if (status_win) delwin(status_win);
        endwin();
    }

    void run() {
        load_history_from_file();
        init_curses();
        create_windows();

        if (client.connect_to_server()) {
            add_to_output("Connected to MinimalRedis server");
        } else {
            add_to_output("Failed to connect to server - press F2 to retry");
        }

        update_status();
        update_output();
        update_input();

        while (running) {
            update_status();

            // Check for pubsub messages in pubsub mode
            if (pubsub_mode && client.is_connected()) {
                std::string msg = client.get_response(100); // Short timeout
                if (!msg.empty()) {
                    add_to_output(msg);
                }
            }

            int ch = getch();
            if (ch != ERR) {
                handle_input(ch);
                update_input();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 6379;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    std::cout << "MinimalRedis TUI Client" << std::endl;
    std::cout << "Connecting to " << host << ":" << port << std::endl;
    std::cout << "Press any key to start..." << std::endl;
    std::cin.get();

    try {
        TUIClient client(host, port);
        client.run();
    } catch (const std::exception& e) {
        endwin();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void TUIClient::update_completion_candidates() {
    completion_candidates.clear();
    completion_index = 0;

    if (current_input.empty()) return;

    size_t word_start = current_input.rfind(' ', cursor_pos);
    if (word_start == std::string::npos) {
        word_start = 0;
    } else {
        word_start++;
    }

    completion_prefix = current_input.substr(word_start, cursor_pos - word_start);

    if (completion_prefix.empty()) return;

    for (const auto& cmd : redis_commands) {
        if (cmd.find(completion_prefix) == 0) {
            completion_candidates.push_back(cmd);
        }
    }

    if (completion_prefix.length() >= 2 && client.is_connected()) {
        std::string keys_cmd = "KEYS " + completion_prefix + "*";
        client.send_command(keys_cmd);
    }
}

void TUIClient::perform_tab_completion() {
    update_completion_candidates();

    if (completion_candidates.empty()) return;

    std::string completion = completion_candidates[completion_index % completion_candidates.size()];

    size_t word_start = current_input.rfind(' ', cursor_pos);
    if (word_start == std::string::npos) {
        word_start = 0;
    } else {
        word_start++;
    }

    size_t word_end = current_input.find(' ', cursor_pos);
    if (word_end == std::string::npos) {
        word_end = current_input.length();
    }

    current_input.replace(word_start, word_end - word_start, completion);
    cursor_pos = word_start + completion.length();

    completion_index++;
}

void TUIClient::save_history_to_file() {
    const char* home = getenv("HOME");
    if (!home) return;

    std::string history_file = std::string(home) + "/.redis_tui_history";

    std::ofstream file(history_file);
    if (!file.is_open()) return;

    size_t start = command_history.size() > 100 ? command_history.size() - 100 : 0;
    for (size_t i = start; i < command_history.size(); ++i) {
        file << command_history[i] << std::endl;
    }
    file.close();
}

void TUIClient::load_history_from_file() {
    const char* home = getenv("HOME");
    if (!home) return;

    std::string history_file = std::string(home) + "/.redis_tui_history";

    std::ifstream file(history_file);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            command_history.push_back(line);
        }
    }
    file.close();

    history_index = command_history.size();
}