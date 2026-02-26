#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <obs-module.h>

struct obs_data_t;
struct obs_source_t;

struct ScheduledItem {
    std::string id;
    std::string name;
    std::string time;           // HH:MM format
    std::string source;         // OBS media source name
    std::string file_path;      // Path to media file
    int duration;               // Duration in seconds (0 = auto-detect)
    bool loop;                  // Whether to loop the media
    std::string scene;          // OBS scene to switch to (optional)
    std::vector<std::string> days; // Days this item is active
    
    // Constructor
    ScheduledItem() : duration(0), loop(false) {}
};

struct Playlist {
    std::string name;
    std::string id;
    std::vector<std::string> days;
    std::vector<ScheduledItem> items;
    bool enabled;
    
    Playlist() : enabled(true) {}
};

class PlaylistManager {
public:
    PlaylistManager();
    ~PlaylistManager();
    
    bool initialize();
    void cleanup();
    
    // Schedule file management
    bool load_schedule_file(const std::string& file_path);
    void unload_schedule_file(const std::string& file_path);
    void reload_schedules();
    
    // Playlist access
    std::vector<Playlist> get_playlists() const;
    Playlist* get_playlist(const std::string& playlist_id);
    
    // Item access
    std::shared_ptr<ScheduledItem> get_item(const std::string& item_id);
    std::vector<std::shared_ptr<ScheduledItem>> get_items_for_time(const std::string& time, const std::string& day) const;
    std::vector<std::shared_ptr<ScheduledItem>> get_items_for_day(const std::string& day) const;
    
    // Validation
    bool validate_schedule_file(const std::string& file_path) const;
    bool validate_item(const ScheduledItem& item) const;
    
    // Status
    size_t get_total_items() const;
    size_t get_active_items() const;
    std::string get_default_idle_content() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, Playlist> playlists_;
    std::map<std::string, std::shared_ptr<ScheduledItem>> items_; // item_id -> item
    std::map<std::string, std::string> file_to_playlist_id_; // file_path -> playlist_id
    std::string default_idle_content_;
    
    // JSON parsing helpers
    bool parse_json_file(const std::string& file_path, Playlist& playlist) const;
    bool parse_playlist_json(const std::string& json_content, Playlist& playlist) const;
    bool parse_item_json(const std::string& json_content, ScheduledItem& item) const;
    
    // Utility functions
    std::string generate_item_id(const ScheduledItem& item) const;
    std::string generate_playlist_id(const std::string& name) const;
    bool time_string_to_minutes(const std::string& time_str, int& minutes) const;
    std::string minutes_to_time_string(int minutes) const;
    std::string get_current_day() const;
    std::string get_current_time() const;
    
    // File monitoring
    void setup_file_watching();
    void cleanup_file_watching();
    
    // Prevent copying
    PlaylistManager(const PlaylistManager&) = delete;
    PlaylistManager& operator=(const PlaylistManager&) = delete;
};
