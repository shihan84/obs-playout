#include "playlist-manager.h"
#include "utils/config.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

PlaylistManager::PlaylistManager() {
}

PlaylistManager::~PlaylistManager() {
    cleanup();
}

bool PlaylistManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Initializing playlist manager");
    
    try {
        // Load all configured schedule files
        auto schedule_files = Config::get_schedule_files();
        for (const auto& file_info : schedule_files) {
            if (file_info.enabled) {
                if (load_schedule_file(file_info.path)) {
                    LOG_INFO("Loaded schedule file: " + file_info.path);
                } else {
                    LOG_WARNING("Failed to load schedule file: " + file_info.path);
                }
            }
        }
        
        // Setup file watching for changes
        setup_file_watching();
        
        LOG_INFO("Playlist manager initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during playlist manager initialization: " + std::string(e.what()));
        return false;
    }
}

void PlaylistManager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    cleanup_file_watching();
    
    playlists_.clear();
    items_.clear();
    file_to_playlist_id_.clear();
    
    LOG_INFO("Playlist manager cleaned up");
}

bool PlaylistManager::load_schedule_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(file_path)) {
            LOG_WARNING("Schedule file does not exist: " + file_path);
            return false;
        }
        
        // Validate file format
        if (!validate_schedule_file(file_path)) {
            LOG_ERROR("Invalid schedule file format: " + file_path);
            return false;
        }
        
        // Parse the file
        Playlist playlist;
        if (!parse_json_file(file_path, playlist)) {
            LOG_ERROR("Failed to parse schedule file: " + file_path);
            return false;
        }
        
        // Remove existing playlist from this file if it exists
        auto it = file_to_playlist_id_.find(file_path);
        if (it != file_to_playlist_id_.end()) {
            auto playlist_it = playlists_.find(it->second);
            if (playlist_it != playlists_.end()) {
                // Remove items from this playlist
                for (const auto& item : playlist_it->second.items) {
                    items_.erase(item.id);
                }
                playlists_.erase(playlist_it);
            }
        }
        
        // Add new playlist
        std::string playlist_id = generate_playlist_id(playlist.name);
        playlist.id = playlist_id;
        playlists_[playlist_id] = playlist;
        file_to_playlist_id_[file_path] = playlist_id;
        
        // Add items to the global item map
        for (auto& item : playlist.items) {
            item.id = generate_item_id(item);
            items_[item.id] = std::make_shared<ScheduledItem>(item);
        }
        
        LOG_INFO("Successfully loaded schedule file: " + file_path + 
                " (Playlist: " + playlist.name + ", Items: " + std::to_string(playlist.items.size()) + ")");
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading schedule file " + file_path + ": " + std::string(e.what()));
        return false;
    }
}

void PlaylistManager::unload_schedule_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = file_to_playlist_id_.find(file_path);
    if (it != file_to_playlist_id_.end()) {
        std::string playlist_id = it->second;
        
        auto playlist_it = playlists_.find(playlist_id);
        if (playlist_it != playlists_.end()) {
            // Remove items from this playlist
            for (const auto& item : playlist_it->second.items) {
                items_.erase(item.id);
            }
            
            playlists_.erase(playlist_it);
            LOG_INFO("Unloaded playlist: " + playlist_id);
        }
        
        file_to_playlist_id_.erase(it);
    }
}

void PlaylistManager::reload_schedules() {
    LOG_INFO("Reloading all schedule files");
    
    // Clear existing data
    playlists_.clear();
    items_.clear();
    file_to_playlist_id_.clear();
    
    // Reload all configured schedule files
    auto schedule_files = Config::get_schedule_files();
    for (const auto& file_info : schedule_files) {
        if (file_info.enabled) {
            load_schedule_file(file_info.path);
        }
    }
}

std::vector<Playlist> PlaylistManager::get_playlists() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Playlist> result;
    result.reserve(playlists_.size());
    
    for (const auto& pair : playlists_) {
        result.push_back(pair.second);
    }
    
    return result;
}

Playlist* PlaylistManager::get_playlist(const std::string& playlist_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = playlists_.find(playlist_id);
    return (it != playlists_.end()) ? &it->second : nullptr;
}

std::shared_ptr<ScheduledItem> PlaylistManager::get_item(const std::string& item_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = items_.find(item_id);
    return (it != items_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<ScheduledItem>> PlaylistManager::get_items_for_time(
    const std::string& time, const std::string& day) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ScheduledItem>> result;
    
    for (const auto& pair : items_) {
        const auto& item = pair.second;
        
        // Check if item is scheduled for this day
        bool day_match = std::find(item->days.begin(), item->days.end(), day) != item->days.end();
        
        // Check if item is scheduled for this time
        bool time_match = (item->time == time);
        
        if (day_match && time_match) {
            result.push_back(item);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<ScheduledItem>> PlaylistManager::get_items_for_day(const std::string& day) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ScheduledItem>> result;
    
    for (const auto& pair : items_) {
        const auto& item = pair.second;
        
        // Check if item is scheduled for this day
        if (std::find(item->days.begin(), item->days.end(), day) != item->days.end()) {
            result.push_back(item);
        }
    }
    
    return result;
}

bool PlaylistManager::validate_schedule_file(const std::string& file_path) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();
        
        // Basic JSON validation - check for required fields
        if (content.find("\"version\"") == std::string::npos) {
            return false;
        }
        
        if (content.find("\"playlists\"") == std::string::npos) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception validating schedule file: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistManager::validate_item(const ScheduledItem& item) const {
    // Check required fields
    if (item.name.empty() || item.time.empty() || item.source.empty()) {
        return false;
    }
    
    // Validate time format (HH:MM)
    std::regex time_regex(R"(^([01]?[0-9]|2[0-3]):[0-5][0-9]$)");
    if (!std::regex_match(item.time, time_regex)) {
        return false;
    }
    
    // Validate file path if provided
    if (!item.file_path.empty() && !std::filesystem::exists(item.file_path)) {
        LOG_WARNING("Media file does not exist: " + item.file_path);
        // Don't fail validation, just warn
    }
    
    return true;
}

size_t PlaylistManager::get_total_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_.size();
}

size_t PlaylistManager::get_active_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string current_day = get_current_day();
    size_t count = 0;
    
    for (const auto& pair : items_) {
        const auto& item = pair.second;
        if (std::find(item->days.begin(), item->days.end(), current_day) != item->days.end()) {
            count++;
        }
    }
    
    return count;
}

std::string PlaylistManager::get_default_idle_content() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return default_idle_content_;
}

bool PlaylistManager::parse_json_file(const std::string& file_path, Playlist& playlist) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();
        
        return parse_playlist_json(content, playlist);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception parsing JSON file: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistManager::parse_playlist_json(const std::string& json_content, Playlist& playlist) const {
    // This is a simplified JSON parser - in a real implementation, use nlohmann/json
    // For now, we'll do basic string parsing
    
    try {
        // Extract playlist name (simplified)
        size_t name_pos = json_content.find("\"name\"");
        if (name_pos != std::string::npos) {
            size_t colon_pos = json_content.find(":", name_pos);
            size_t value_start = json_content.find("\"", colon_pos);
            if (value_start != std::string::npos) {
                size_t value_end = json_content.find("\"", value_start + 1);
                if (value_end != std::string::npos) {
                    playlist.name = json_content.substr(value_start + 1, value_end - value_start - 1);
                }
            }
        }
        
        // Extract default idle content
        size_t idle_pos = json_content.find("\"default_idle\"");
        if (idle_pos != std::string::npos) {
            size_t colon_pos = json_content.find(":", idle_pos);
            size_t value_start = json_content.find("\"", colon_pos);
            if (value_start != std::string::npos) {
                size_t value_end = json_content.find("\"", value_start + 1);
                if (value_end != std::string::npos) {
                    default_idle_content_ = json_content.substr(value_start + 1, value_end - value_start - 1);
                }
            }
        }
        
        // Extract days (simplified)
        std::vector<std::string> days = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"};
        playlist.days = days; // Default to all days
        
        // For now, create a sample item - in real implementation, parse items array
        ScheduledItem sample_item;
        sample_item.name = "Sample Item";
        sample_item.time = "09:00";
        sample_item.source = "Media Source";
        sample_item.file_path = default_idle_content_;
        sample_item.days = days;
        playlist.items.push_back(sample_item);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception parsing playlist JSON: " + std::string(e.what()));
        return false;
    }
}

std::string PlaylistManager::generate_item_id(const ScheduledItem& item) const {
    return item.source + "_" + item.time + "_" + std::to_string(std::hash<std::string>{}(item.name));
}

std::string PlaylistManager::generate_playlist_id(const std::string& name) const {
    return "playlist_" + std::to_string(std::hash<std::string>{}(name));
}

void PlaylistManager::setup_file_watching() {
    // File watching implementation would go here
    // For now, we'll just log that it's set up
    LOG_INFO("File watching setup (placeholder)");
}

void PlaylistManager::cleanup_file_watching() {
    // File watching cleanup would go here
    LOG_INFO("File watching cleanup");
}

std::string PlaylistManager::get_current_day() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    static const std::vector<std::string> days = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
    return days[tm.tm_wday];
}

std::string PlaylistManager::get_current_time() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M");
    return ss.str();
}
