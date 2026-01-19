#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

class SaveDaemon {
private:
    std::thread worker_thread;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::atomic<bool> stop_requested{false};
    
    // Configuration
    std::chrono::seconds save_interval{30};
    std::string db_file;
    std::function<bool(const std::string&)> save_function;
    
    // Statistics
    std::atomic<int> successful_saves{0};
    std::atomic<int> failed_saves{0};
    std::chrono::steady_clock::time_point last_save_time;
    
    void worker_loop();
    void perform_save();

public:
    SaveDaemon(const std::string& file, std::function<bool(const std::string&)> save_func);
    ~SaveDaemon();
    
    // Control methods
    void start();
    void stop();
    void trigger_save(); // Force immediate save
    
    // Configuration
    void set_interval(std::chrono::seconds interval);
    
    // Status
    bool is_running() const { return running; }
    int get_successful_saves() const { return successful_saves; }
    int get_failed_saves() const { return failed_saves; }
    std::chrono::seconds get_interval() const { return save_interval; }
};