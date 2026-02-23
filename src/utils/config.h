#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>

class Config {
public:
    struct ScheduleFile {
        std::string path;
        bool enabled;
        std::string name;
    };

    static void load();
    static void save();
    
    static std::string get_config_path();
    static std::string get_default_schedule_path();
    
    // Schedule file management
    static std::vector<ScheduleFile> get_schedule_files();
    static void add_schedule_file(const ScheduleFile& file);
    static void remove_schedule_file(const std::string& path);
    static void update_schedule_file(const ScheduleFile& file);
    
    // General settings
    static bool is_enabled();
    static void set_enabled(bool enabled);
    
    static int get_check_interval_seconds();
    static void set_check_interval_seconds(int interval);
    
    static std::string get_timezone();
    static void set_timezone(const std::string& timezone);
    
    static bool is_debug_mode();
    static void set_debug_mode(bool enabled);

private:
    static std::mutex mutex_;
    static std::string config_path_;
    
    // Configuration data
    static bool enabled_;
    static int check_interval_seconds_;
    static std::string timezone_;
    static bool debug_mode_;
    static std::vector<ScheduleFile> schedule_files_;
    
    static void load_default_config();
    static std::string escape_json_string(const std::string& str);
    static std::string unescape_json_string(const std::string& str);
};
