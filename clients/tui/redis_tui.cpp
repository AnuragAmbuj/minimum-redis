#include "redis_client.h"
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
#include <unordered_map>
#include <iomanip>
#include <algorithm>
#include <chrono>

// Enhanced TUI Client with multiple views
class EnhancedRedisTUI {
private:
    RedisClient client;

    // Data Browser
    std::vector<std::pair<std::string, std::string>> key_value_pairs;
    size_t current_page = 0;
    const size_t page_size = 20;

    // Server Monitor
    std::unordered_map<std::string, std::string> server_info;
    std::vector<double> memory_history;
    std::vector<int> connections_history;
    const size_t max_history = 20;

    // Command Interface
    std::vector<std::string> command_history;
    std::vector<std::string> response_history;
    std::string current_command;
    const size_t max_history_cmds = 100;

    int current_view = 0; // 0: menu, 1: command, 2: data, 3: monitor
    bool running = true;

    void display_header() {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "MinimalRedis Unified TUI Client v2.0" << std::endl;
        std::cout << "Connected: " << (client.is_connected() ? "[YES] " : "[NO] ")
                 << client.get_host() << ":" << client.get_port() << std::endl;
        std::cout << std::string(70, '=') << std::endl;
    }

    void display_menu() {
        display_header();
        std::cout << "\nMAIN MENU" << std::endl;
        std::cout << "1. Command Interface  - Execute Redis commands" << std::endl;
        std::cout << "2. Data Browser       - View and browse keys" << std::endl;
        std::cout << "3. Server Monitor     - Real-time server stats" << std::endl;
        std::cout << "4. Connection         - Connect/Disconnect" << std::endl;
        std::cout << "5. Command History    - View previous commands" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "\nChoose option (0-5): ";
    }

    void handle_command_interface() {
        display_header();
        std::cout << "\nCOMMAND INTERFACE" << std::endl;
        std::cout << "Available commands: SET, GET, DEL, KEYS, INFO, etc." << std::endl;
        std::cout << "Type 'back' to return to menu" << std::endl;
        std::cout << "\nEnter command: ";

        std::string cmd;
        std::getline(std::cin, cmd);

        if (cmd == "back") return;

        if (cmd.empty()) {
            std::cout << "[WARNING]  Empty command" << std::endl;
            return;
        }

        command_history.push_back(cmd);
        if (command_history.size() > max_history_cmds) {
            command_history.erase(command_history.begin());
        }

        std::cout << "\n[EXEC]  Executing: " << cmd << std::endl;

        if (!client.send_command(cmd)) {
            std::cout << "[ERROR] Failed to send command" << std::endl;
            return;
        }

        std::string response = client.get_response();
        if (!response.empty()) {
            response_history.push_back(response);
            std::cout << "[RESPONSE] Response: " << response << std::endl;
        } else {
            std::cout << "[TIMEOUT] Timeout or no response" << std::endl;
        }

        std::cout << "\nPress Enter to continue...";
        std::cin.get();
    }

    void refresh_data() {
        key_value_pairs.clear();

        if (!client.send_command("KEYS *")) return;

        std::string keys_response = client.get_response();
        if (keys_response.empty()) return;

        std::vector<std::string> keys;
        std::stringstream ss(keys_response);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.size() > 1 && line[0] == '$') {
                std::getline(ss, line);
                if (!line.empty()) keys.push_back(line);
            }
        }

        for (size_t i = 0; i < std::min(keys.size(), size_t(50)); ++i) {
            if (client.send_command("GET " + keys[i])) {
                std::string value = client.get_response();
                if (!value.empty() && value[0] != '-') {
                    key_value_pairs.emplace_back(keys[i], value);
                }
            }
        }
    }

    void handle_data_browser() {
        refresh_data();
        display_header();

        std::cout << "\n[DATA] DATA BROWSER" << std::endl;
        std::cout << "Total keys found: " << key_value_pairs.size() << std::endl;

        if (key_value_pairs.empty()) {
            std::cout << "[EMPTY] No keys found in database" << std::endl;
        } else {
            std::cout << std::string(80, '-') << std::endl;

            size_t start = current_page * page_size;
            size_t end = std::min(start + page_size, key_value_pairs.size());

            for (size_t i = start; i < end; ++i) {
                const auto& [key, value] = key_value_pairs[i];
                std::cout << std::setw(3) << (i + 1) << ". "
                         << std::left << std::setw(25) << key.substr(0, 25)
                         << " → " << value.substr(0, 45) << std::endl;
            }

            if (key_value_pairs.size() > page_size) {
                std::cout << "\n[PAGE] Page " << (current_page + 1) << " of "
                         << ((key_value_pairs.size() + page_size - 1) / page_size) << std::endl;
            }
        }

        std::cout << "\nCommands: n(ext), p(rev), r(efresh), b(ack): ";
        char cmd;
        std::cin >> cmd;
        std::cin.ignore(); // Clear newline

        switch (cmd) {
            case 'n': if ((current_page + 1) * page_size < key_value_pairs.size()) current_page++; break;
            case 'p': if (current_page > 0) current_page--; break;
            case 'r': break; // Just refresh
            case 'b': return;
        }
    }

    void fetch_server_info() {
        server_info.clear();

        if (!client.send_command("INFO")) return;

        std::string info_response = client.get_response(2000);
        if (info_response.empty()) return;

        std::stringstream ss(info_response);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty() || line[0] == '#') continue;

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                server_info[key] = value;
            }
        }
    }

    void update_monitoring_data() {
        auto it = server_info.find("used_memory");
        if (it != server_info.end()) {
            try {
                double mem = std::stod(it->second) / 1024.0 / 1024.0;
                memory_history.push_back(mem);
                if (memory_history.size() > max_history) {
                    memory_history.erase(memory_history.begin());
                }
            } catch (...) {}
        }

        it = server_info.find("connected_clients");
        if (it != server_info.end()) {
            try {
                int clients = std::stoi(it->second);
                connections_history.push_back(clients);
                if (connections_history.size() > max_history) {
                    connections_history.erase(connections_history.begin());
                }
            } catch (...) {}
        }
    }

    void handle_server_monitor() {
        fetch_server_info();
        update_monitoring_data();

        display_header();
        std::cout << "\n[MONITOR] SERVER MONITOR" << std::endl;

        if (server_info.empty()) {
            std::cout << "[ERROR] No server information available" << std::endl;
            std::cout << "Make sure you're connected to a MinimalRedis server" << std::endl;
        } else {
            std::cout << "[INFO] Server Info:" << std::endl;
            std::cout << "  Redis Version: " << server_info["redis_version"] << std::endl;
            std::cout << "  Uptime: " << server_info["uptime_in_seconds"] << " seconds" << std::endl;
            std::cout << "  Connected Clients: " << server_info["connected_clients"] << std::endl;

            if (!server_info["used_memory"].empty()) {
                double mem_mb = std::stod(server_info["used_memory"]) / 1024.0 / 1024.0;
                std::cout << "  Memory Used: " << std::fixed << std::setprecision(2) << mem_mb << " MB" << std::endl;
            }

            std::cout << "  Total Commands: " << server_info["total_commands_processed"] << std::endl;

            if (!memory_history.empty()) {
                std::cout << "\n[DATA] Memory Usage Trend:" << std::endl;
                double max_mem = *std::max_element(memory_history.begin(), memory_history.end());
                for (size_t i = 0; i < memory_history.size(); ++i) {
                    int bars = max_mem > 0 ? (memory_history[i] / max_mem) * 30 : 0;
                    std::cout << std::setw(2) << i << ": "
                             << std::string(bars, '#') << " "
                             << std::fixed << std::setprecision(1) << memory_history[i] << " MB" << std::endl;
                }
            }

            if (!connections_history.empty()) {
                std::cout << "\n[CLIENTS] Connection Trend:" << std::endl;
                int max_conn = *std::max_element(connections_history.begin(), connections_history.end());
                for (size_t i = 0; i < connections_history.size(); ++i) {
                    int bars = max_conn > 0 ? (connections_history[i] * 30) / max_conn : 0;
                    std::cout << std::setw(2) << i << ": "
                             << std::string(bars, '*') << " "
                             << connections_history[i] << " clients" << std::endl;
                }
            }
        }

        std::cout << "\nPress Enter to refresh or 'b' for menu: ";
        std::string input;
        std::getline(std::cin, input);
        if (input != "b") {
            // Stay in monitor mode for continuous updates
            handle_server_monitor();
        }
    }

    void handle_connection() {
        display_header();
        std::cout << "\n[CONNECTION] CONNECTION MANAGEMENT" << std::endl;

        if (client.is_connected()) {
            std::cout << "Currently connected to " << client.get_host() << ":" << client.get_port() << std::endl;
            std::cout << "Type 'disconnect' to disconnect, or 'back' to return: ";

            std::string cmd;
            std::getline(std::cin, cmd);

            if (cmd == "disconnect") {
                client.stop();
                std::cout << "[OK] Disconnected successfully" << std::endl;
            }
        } else {
            std::cout << "Not connected. Type 'connect' to connect to default server, or 'back': ";

            std::string cmd;
            std::getline(std::cin, cmd);

            if (cmd == "connect") {
                if (client.connect()) {
                    std::cout << "[OK] Connected to " << client.get_host() << ":" << client.get_port() << std::endl;
                } else {
                    std::cout << "[ERROR] Failed to connect" << std::endl;
                }
            }
        }

        std::cout << "\nPress Enter to continue...";
        std::cin.get();
    }

    void handle_history() {
        display_header();
        std::cout << "\n[HISTORY] COMMAND HISTORY" << std::endl;

        if (command_history.empty()) {
            std::cout << "No commands in history" << std::endl;
        } else {
            for (size_t i = 0; i < command_history.size(); ++i) {
                std::cout << std::setw(3) << (i + 1) << ". " << command_history[i] << std::endl;
            }
        }

        std::cout << "\nPress Enter to continue...";
        std::cin.get();
    }

public:
    EnhancedRedisTUI(const std::string& host = "127.0.0.1", int port = 6379)
        : client(host, port) {}

    ~EnhancedRedisTUI() {
        client.stop();
    }

    void run() {
        std::cout << "[TARGET] Welcome to MinimalRedis Unified TUI Client!" << std::endl;
        std::cout << "This client provides a comprehensive interface for Redis operations." << std::endl;
        std::cout << "\nPress Enter to start...";
        std::cin.get();

        while (running) {
            display_menu();

            std::string choice;
            std::getline(std::cin, choice);

            if (choice.empty()) continue;

            switch (choice[0]) {
                case '1':
                    handle_command_interface();
                    break;
                case '2':
                    handle_data_browser();
                    break;
                case '3':
                    handle_server_monitor();
                    break;
                case '4':
                    handle_connection();
                    break;
                case '5':
                    handle_history();
                    break;
                case '0':
                    running = false;
                    break;
                default:
                    std::cout << "[ERROR] Invalid choice. Please select 0-5." << std::endl;
                    std::cout << "Press Enter to continue...";
                    std::cin.get();
            }
        }

        std::cout << "\n[BYE] Thank you for using MinimalRedis Unified TUI Client!" << std::endl;
        std::cout << "Have a great day! [ROCKET]" << std::endl;
    }
};



int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 6379;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) {
        try {
            port = std::stoi(argv[2]);
        } catch (const std::exception&) {
            std::cerr << "Invalid port number: " << argv[2] << std::endl;
            return 1;
        }
    }

    try {
        EnhancedRedisTUI app(host, port);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}