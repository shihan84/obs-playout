#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>

class PlaylistManager;
class MediaController;
class TimeTrigger;

class SchedulerCore {
public:
    SchedulerCore();
    ~SchedulerCore();
    
    bool initialize();
    void start();
    void stop();
    void toggle_enabled();
    
    bool is_running() const;
    bool is_enabled() const;
    
    void reload_schedules();
    void force_check();
    
    // Status information
    std::string get_status() const;
    std::string get_current_item() const;
    std::string get_next_item() const;

private:
    void scheduler_loop();
    void check_and_execute_schedules();
    void execute_scheduled_item(const std::string& item_id);
    
    std::unique_ptr<std::thread> scheduler_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> enabled_;
    std::atomic<bool> should_reload_;
    
    std::unique_ptr<PlaylistManager> playlist_manager_;
    std::unique_ptr<MediaController> media_controller_;
    std::unique_ptr<TimeTrigger> time_trigger_;
    
    mutable std::mutex status_mutex_;
    std::string current_item_id_;
    std::string next_item_id_;
    std::chrono::steady_clock::time_point last_check_time_;
    
    std::condition_variable cv_;
    std::mutex cv_mutex_;
    
    // Configuration
    int check_interval_seconds_;
    
    // Prevent copying
    SchedulerCore(const SchedulerCore&) = delete;
    SchedulerCore& operator=(const SchedulerCore&) = delete;
};
