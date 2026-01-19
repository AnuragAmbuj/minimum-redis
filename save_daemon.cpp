#include "save_daemon.h"
#include <iostream>
#include <unistd.h>

SaveDaemon::SaveDaemon(const std::string& file, std::function<bool(const std::string&)> save_func)
    : db_file(file), save_function(save_func) {
}

SaveDaemon::~SaveDaemon() {
    stop();
}

void SaveDaemon::start() {
    if (running) return;
    
    stop_requested = false;
    running = true;
    worker_thread = std::thread(&SaveDaemon::worker_loop, this);
}

void SaveDaemon::stop() {
    if (!running) return;
    
    stop_requested = true;
    cv.notify_all();
    
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    
    running = false;
}

void SaveDaemon::trigger_save() {
    // Force an immediate save by notifying the worker
    cv.notify_all();
}

void SaveDaemon::set_interval(std::chrono::seconds interval) {
    std::lock_guard<std::mutex> lock(mutex);
    save_interval = interval;
}

void SaveDaemon::worker_loop() {
    last_save_time = std::chrono::steady_clock::now();
    
    while (!stop_requested) {
        std::unique_lock<std::mutex> lock(mutex);
        
        // Wait for either timeout or notification
        auto wait_result = cv.wait_for(lock, save_interval);
        
        if (stop_requested) break;
        
        // Check if it's time for a periodic save
        auto now = std::chrono::steady_clock::now();
        auto time_since_last_save = now - last_save_time;
        
        if (time_since_last_save >= save_interval || wait_result == std::cv_status::no_timeout) {
            perform_save();
            last_save_time = std::chrono::steady_clock::now();
        }
    }
}

void SaveDaemon::perform_save() {
    try {
        bool success = save_function(db_file);
        if (success) {
            successful_saves++;
            std::cout << "[SaveDaemon] Database saved successfully (" 
                     << successful_saves << " total)" << std::endl;
        } else {
            failed_saves++;
            std::cerr << "[SaveDaemon] Database save failed (" 
                     << failed_saves << " failures)" << std::endl;
            
            // Exponential backoff on failure (don't spam the logs)
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    } catch (const std::exception& e) {
        failed_saves++;
        std::cerr << "[SaveDaemon] Exception during save: " << e.what() << std::endl;
    }
}