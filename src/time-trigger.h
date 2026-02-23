#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <mutex>

class PlaylistManager;

struct TimeSlot {
    int hour;
    int minute;
    std::vector<std::string> item_ids;
    
    TimeSlot(int h = 0, int m = 0) : hour(h), minute(m) {}
    
    bool operator<(const TimeSlot& other) const {
        if (hour != other.hour) return hour < other.hour;
        return minute < other.minute;
    }
    
    int to_minutes() const {
        return hour * 60 + minute;
    }
    
    std::string to_string() const {
        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
        return std::string(buffer);
    }
};

class TimeTrigger {
public:
    TimeTrigger();
    ~TimeTrigger();
    
    bool initialize();
    void cleanup();
    
    // Core functionality
    void update_schedule();
    std::vector<std::string> get_current_items();
    std::vector<std::string> get_next_items();
    std::vector<std::string> get_upcoming_items(int count = 5);
    
    // Time utilities
    std::string get_current_time() const;
    std::string get_current_day() const;
    int get_current_minutes() const;
    
    // Schedule management
    void rebuild_schedule();
    void clear_schedule();
    
    // Status
    size_t get_schedule_size() const;
    bool is_schedule_empty() const;
    std::string get_next_trigger_time() const;

private:
    mutable std::mutex mutex_;
    std::vector<TimeSlot> schedule_;
    std::unique_ptr<PlaylistManager> playlist_manager_;
    
    // Cache for current state
    std::string cached_day_;
    int cached_minutes_;
    std::chrono::steady_clock::time_point last_update_;
    
    // Configuration
    std::string timezone_;
    int check_tolerance_seconds_; // How close to the exact time we should trigger
    
    // Internal methods
    void update_cache();
    std::vector<std::string> get_items_at_time(int minutes) const;
    std::vector<std::string> get_items_after_time(int minutes, int max_count = 10) const;
    
    // Time conversion utilities
    int time_string_to_minutes(const std::string& time_str) const;
    std::string minutes_to_time_string(int minutes) const;
    bool is_same_day(const std::string& day1, const std::string& day2) const;
    
    // Schedule building
    void build_schedule_from_playlists();
    void add_items_to_schedule(const std::vector<std::string>& item_ids, int time_minutes);
    
    // Prevent copying
    TimeTrigger(const TimeTrigger&) = delete;
    TimeTrigger& operator=(const TimeTrigger&) = delete;
};
