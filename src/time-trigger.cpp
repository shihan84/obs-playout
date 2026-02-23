#include "time-trigger.h"
#include "playlist-manager.h"
#include "utils/config.h"
#include "utils/logger.h"
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

TimeTrigger::TimeTrigger()
    : cached_minutes_(-1)
    , last_update_(std::chrono::steady_clock::time_point::min())
    , check_tolerance_seconds_(30)
{
}

TimeTrigger::~TimeTrigger() {
    cleanup();
}

bool TimeTrigger::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Initializing time trigger");
    
    try {
        // Get playlist manager instance
        playlist_manager_ = std::make_unique<PlaylistManager>();
        if (!playlist_manager_->initialize()) {
            LOG_ERROR("Failed to initialize playlist manager in time trigger");
            return false;
        }
        
        // Load configuration
        timezone_ = Config::get_timezone();
        
        // Build initial schedule
        rebuild_schedule();
        
        // Initialize cache
        update_cache();
        
        LOG_INFO("Time trigger initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during time trigger initialization: " + std::string(e.what()));
        return false;
    }
}

void TimeTrigger::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (playlist_manager_) {
        playlist_manager_->cleanup();
        playlist_manager_.reset();
    }
    
    schedule_.clear();
    
    LOG_INFO("Time trigger cleaned up");
}

void TimeTrigger::update_schedule() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we need to update (day changed or schedule is empty)
    update_cache();
    
    if (schedule_.empty() || !is_same_day(cached_day_, get_current_day())) {
        rebuild_schedule();
        update_cache();
    }
}

std::vector<std::string> TimeTrigger::get_current_items() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    update_schedule();
    
    int current_minutes = get_current_minutes();
    return get_items_at_time(current_minutes);
}

std::vector<std::string> TimeTrigger::get_next_items() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    update_schedule();
    
    int current_minutes = get_current_minutes();
    auto items = get_items_after_time(current_minutes, 1);
    
    return items;
}

std::vector<std::string> TimeTrigger::get_upcoming_items(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    update_schedule();
    
    int current_minutes = get_current_minutes();
    return get_items_after_time(current_minutes, count);
}

std::string TimeTrigger::get_current_time() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M");
    return ss.str();
}

std::string TimeTrigger::get_current_day() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    static const std::vector<std::string> days = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
    return days[tm.tm_wday];
}

int TimeTrigger::get_current_minutes() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    return tm.tm_hour * 60 + tm.tm_min;
}

void TimeTrigger::rebuild_schedule() {
    LOG_INFO("Rebuilding time schedule");
    
    schedule_.clear();
    
    if (!playlist_manager_) {
        LOG_ERROR("Playlist manager not initialized");
        return;
    }
    
    try {
        std::string current_day = get_current_day();
        
        // Get all items for current day
        auto day_items = playlist_manager_->get_items_for_day(current_day);
        
        // Group items by time
        std::map<int, std::vector<std::string>> time_to_items;
        
        for (const auto& item : day_items) {
            int time_minutes = time_string_to_minutes(item->time);
            if (time_minutes >= 0 && time_minutes < 24 * 60) {
                time_to_items[time_minutes].push_back(item->id);
            }
        }
        
        // Build schedule vector
        for (const auto& pair : time_to_items) {
            TimeSlot slot;
            slot.hour = pair.first / 60;
            slot.minute = pair.first % 60;
            slot.item_ids = pair.second;
            schedule_.push_back(slot);
        }
        
        // Sort schedule by time
        std::sort(schedule_.begin(), schedule_.end());
        
        LOG_INFO("Schedule rebuilt with " + std::to_string(schedule_.size()) + " time slots");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception rebuilding schedule: " + std::string(e.what()));
    }
}

void TimeTrigger::clear_schedule() {
    std::lock_guard<std::mutex> lock(mutex_);
    schedule_.clear();
    LOG_INFO("Schedule cleared");
}

size_t TimeTrigger::get_schedule_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return schedule_.size();
}

bool TimeTrigger::is_schedule_empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return schedule_.empty();
}

std::string TimeTrigger::get_next_trigger_time() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (schedule_.empty()) {
        return "No schedule";
    }
    
    int current_minutes = get_current_minutes();
    
    for (const auto& slot : schedule_) {
        int slot_minutes = slot.to_minutes();
        if (slot_minutes > current_minutes) {
            return slot.to_string();
        }
    }
    
    // If no slot found for today, return first slot for tomorrow
    if (!schedule_.empty()) {
        return schedule_[0].to_string() + " (tomorrow)";
    }
    
    return "No schedule";
}

void TimeTrigger::update_cache() {
    cached_day_ = get_current_day();
    cached_minutes_ = get_current_minutes();
    last_update_ = std::chrono::steady_clock::now();
}

std::vector<std::string> TimeTrigger::get_items_at_time(int minutes) const {
    std::vector<std::string> result;
    
    for (const auto& slot : schedule_) {
        int slot_minutes = slot.to_minutes();
        
        // Check if this slot should trigger now (within tolerance)
        int diff = std::abs(minutes - slot_minutes);
        if (diff <= check_tolerance_seconds_ / 60) {
            result.insert(result.end(), slot.item_ids.begin(), slot.item_ids.end());
        }
    }
    
    return result;
}

std::vector<std::string> TimeTrigger::get_items_after_time(int minutes, int max_count) const {
    std::vector<std::string> result;
    
    for (const auto& slot : schedule_) {
        int slot_minutes = slot.to_minutes();
        
        if (slot_minutes > minutes) {
            result.insert(result.end(), slot.item_ids.begin(), slot.item_ids.end());
            
            if (result.size() >= static_cast<size_t>(max_count)) {
                break;
            }
        }
    }
    
    return result;
}

int TimeTrigger::time_string_to_minutes(const std::string& time_str) const {
    try {
        size_t colon_pos = time_str.find(':');
        if (colon_pos == std::string::npos) {
            return -1;
        }
        
        std::string hour_str = time_str.substr(0, colon_pos);
        std::string minute_str = time_str.substr(colon_pos + 1);
        
        int hour = std::stoi(hour_str);
        int minute = std::stoi(minute_str);
        
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
            return -1;
        }
        
        return hour * 60 + minute;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception parsing time string '" + time_str + "': " + std::string(e.what()));
        return -1;
    }
}

std::string TimeTrigger::minutes_to_time_string(int minutes) const {
    if (minutes < 0 || minutes >= 24 * 60) {
        return "00:00";
    }
    
    int hour = minutes / 60;
    int minute = minutes % 60;
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hour << ":" << std::setfill('0') << std::setw(2) << minute;
    return ss.str();
}

bool TimeTrigger::is_same_day(const std::string& day1, const std::string& day2) const {
    return day1 == day2;
}
