#include "ui_components.h"
#include <iomanip>
#include <algorithm>

DataBrowser::DataBrowser(RedisClient& c) : client(c) {}

void DataBrowser::refresh_data() {
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

void DataBrowser::display() {
    refresh_data();

    std::cout << "\n=== DATA BROWSER ===" << std::endl;
    std::cout << "Total keys: " << key_value_pairs.size() << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    size_t start = current_page * page_size;
    size_t end = std::min(start + page_size, key_value_pairs.size());

    for (size_t i = start; i < end; ++i) {
        const auto& [key, value] = key_value_pairs[i];
        std::cout << std::setw(3) << (i + 1) << ". "
                 << std::left << std::setw(20) << key.substr(0, 20)
                 << " -> " << value.substr(0, 40) << std::endl;
    }

    if (key_value_pairs.size() > page_size) {
        std::cout << "\nPage " << (current_page + 1) << " of "
                 << ((key_value_pairs.size() + page_size - 1) / page_size) << std::endl;
    }
}

void DataBrowser::next_page() {
    if ((current_page + 1) * page_size < key_value_pairs.size()) {
        current_page++;
    }
}

void DataBrowser::prev_page() {
    if (current_page > 0) current_page--;
}

CommandInterface::CommandInterface(RedisClient& c) : client(c) {}

void CommandInterface::execute_command() {
    if (current_command.empty()) return;

    command_history.push_back(current_command);
    if (command_history.size() > max_history) {
        command_history.erase(command_history.begin());
    }
    history_index = command_history.size();

    std::cout << "\n> " << current_command << std::endl;

    if (!client.send_command(current_command)) {
        std::cout << "Failed to send command" << std::endl;
        return;
    }

    std::string response = client.get_response();
    if (!response.empty()) {
        response_history.push_back(response);
        std::cout << response << std::endl;
    } else {
        std::cout << "(timeout)" << std::endl;
    }

    current_command.clear();
}

void CommandInterface::display() {
    std::cout << "\n=== COMMAND INTERFACE ===" << std::endl;
    std::cout << "Current command: " << current_command << std::endl;
    std::cout << "History size: " << command_history.size() << std::endl;
    std::cout << "\nAvailable commands: SET, GET, DEL, KEYS, INFO, etc." << std::endl;
    std::cout << "Type 'help' for command help" << std::endl;
}

void CommandInterface::input_command(const std::string& cmd) {
    current_command = cmd;
    execute_command();
}

void CommandInterface::show_history() {
    std::cout << "\n=== COMMAND HISTORY ===" << std::endl;
    for (size_t i = 0; i < command_history.size(); ++i) {
        std::cout << std::setw(3) << (i + 1) << ". " << command_history[i] << std::endl;
    }
}

ServerMonitor::ServerMonitor(RedisClient& c) : client(c) {}

void ServerMonitor::fetch_server_info() {
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

void ServerMonitor::update_metrics() {
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

void ServerMonitor::display() {
    std::cout << "\n=== SERVER MONITOR ===" << std::endl;

    if (server_info.empty()) {
        std::cout << "No server information available" << std::endl;
        return;
    }

    std::cout << "Redis Version: " << server_info["redis_version"] << std::endl;
    std::cout << "Uptime: " << server_info["uptime_in_seconds"] << " seconds" << std::endl;
    std::cout << "Connected Clients: " << server_info["connected_clients"] << std::endl;

    if (!server_info["used_memory"].empty()) {
        double mem_mb = std::stod(server_info["used_memory"]) / 1024.0 / 1024.0;
        std::cout << "Used Memory: " << std::fixed << std::setprecision(2) << mem_mb << " MB" << std::endl;
    }

    std::cout << "Total Commands: " << server_info["total_commands_processed"] << std::endl;

    if (!memory_history.empty()) {
        std::cout << "\nMemory Usage (last " << memory_history.size() << " samples):" << std::endl;
        double max_mem = *std::max_element(memory_history.begin(), memory_history.end());
        for (size_t i = 0; i < memory_history.size(); ++i) {
            int bars = max_mem > 0 ? (memory_history[i] / max_mem) * 20 : 0;
            std::cout << std::setw(2) << i << ": " << std::string(bars, '#') << std::endl;
        }
    }

    if (!connections_history.empty()) {
        std::cout << "\nConnections (last " << connections_history.size() << " samples):" << std::endl;
        int max_conn = *std::max_element(connections_history.begin(), connections_history.end());
        for (size_t i = 0; i < connections_history.size(); ++i) {
            int bars = max_conn > 0 ? (connections_history[i] * 20) / max_conn : 0;
            std::cout << std::setw(2) << i << ": " << std::string(bars, '*') << std::endl;
        }
    }
}

void ServerMonitor::update() {
    fetch_server_info();
    update_metrics();
}