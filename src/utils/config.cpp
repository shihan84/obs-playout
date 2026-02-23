#include "config.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

std::mutex Config::mutex_;
std::string Config::config_path_;

bool Config::enabled_ = true;
int Config::check_interval_seconds_ = 1;
std::string Config::timezone_ = "UTC";
bool Config::debug_mode_ = false;
std::vector<Config::ScheduleFile> Config::schedule_files_;

void Config::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    config_path_ = get_config_path();
    
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        LOG_INFO("Config file not found, creating default configuration");
        load_default_config();
        save();
        return;
    }
    
    try {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();
        
        // Simple JSON parsing (basic implementation)
        // In a real implementation, you'd use nlohmann/json
        
        // Parse enabled
        size_t enabled_pos = content.find("\"enabled\":");
        if (enabled_pos != std::string::npos) {
            size_t colon_pos = content.find(":", enabled_pos);
            size_t value_start = content.find_first_not_of(" \t\n\r", colon_pos + 1);
            if (value_start != std::string::npos) {
                enabled_ = content.substr(value_start, 4) == "true";
            }
        }
        
        // Parse check interval
        size_t interval_pos = content.find("\"check_interval_seconds\":");
        if (interval_pos != std::string::npos) {
            size_t colon_pos = content.find(":", interval_pos);
            size_t value_start = content.find_first_not_of(" \t\n\r", colon_pos + 1);
            if (value_start != std::string::npos) {
                std::string value = content.substr(value_start);
                size_t value_end = value.find_first_of(",}\n\r");
                if (value_end != std::string::npos) {
                    value = value.substr(0, value_end);
                    check_interval_seconds_ = std::stoi(value);
                }
            }
        }
        
        // Parse timezone
        size_t timezone_pos = content.find("\"timezone\":");
        if (timezone_pos != std::string::npos) {
            size_t colon_pos = content.find(":", timezone_pos);
            size_t value_start = content.find_first_not_of(" \t\n\r\"", colon_pos + 1);
            if (value_start != std::string::npos) {
                size_t value_end = content.find("\"", value_start);
                if (value_end != std::string::npos) {
                    timezone_ = content.substr(value_start, value_end - value_start);
                }
            }
        }
        
        // Parse debug mode
        size_t debug_pos = content.find("\"debug_mode\":");
        if (debug_pos != std::string::npos) {
            size_t colon_pos = content.find(":", debug_pos);
            size_t value_start = content.find_first_not_of(" \t\n\r", colon_pos + 1);
            if (value_start != std::string::npos) {
                debug_mode_ = content.substr(value_start, 4) == "true";
            }
        }
        
        LOG_INFO("Configuration loaded successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file: " + std::string(e.what()));
        load_default_config();
    }
}

void Config::save() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream file(config_path_);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file for writing: " + config_path_);
        return;
    }
    
    try {
        file << "{\n";
        file << "  \"enabled\": " << (enabled_ ? "true" : "false") << ",\n";
        file << "  \"check_interval_seconds\": " << check_interval_seconds_ << ",\n";
        file << "  \"timezone\": \"" << escape_json_string(timezone_) << "\",\n";
        file << "  \"debug_mode\": " << (debug_mode_ ? "true" : "false") << ",\n";
        file << "  \"schedule_files\": [\n";
        
        for (size_t i = 0; i < schedule_files_.size(); ++i) {
            const auto& sched_file = schedule_files_[i];
            file << "    {\n";
            file << "      \"path\": \"" << escape_json_string(sched_file.path) << "\",\n";
            file << "      \"enabled\": " << (sched_file.enabled ? "true" : "false") << ",\n";
            file << "      \"name\": \"" << escape_json_string(sched_file.name) << "\"\n";
            file << "    }";
            if (i < schedule_files_.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        LOG_INFO("Configuration saved successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save config file: " + std::string(e.what()));
    }
}

std::string Config::get_config_path() {
#ifdef _WIN32
    char app_data[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, app_data) == S_OK) {
        return std::string(app_data) + "\\obs-time-scheduler\\config.json";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/obs-time-scheduler/config.json";
    }
#endif
    return "config.json";
}

std::string Config::get_default_schedule_path() {
#ifdef _WIN32
    char documents[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documents) == S_OK) {
        return std::string(documents) + "\\OBS Time Scheduler\\schedule.json";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Documents/OBS Time Scheduler/schedule.json";
    }
#endif
    return "schedule.json";
}

std::vector<Config::ScheduleFile> Config::get_schedule_files() {
    std::lock_guard<std::mutex> lock(mutex_);
    return schedule_files_;
}

void Config::add_schedule_file(const ScheduleFile& file) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if file already exists
    auto it = std::find_if(schedule_files_.begin(), schedule_files_.end(),
        [&file](const ScheduleFile& existing) { return existing.path == file.path; });
    
    if (it == schedule_files_.end()) {
        schedule_files_.push_back(file);
        save();
        LOG_INFO("Added schedule file: " + file.path);
    }
}

void Config::remove_schedule_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(schedule_files_.begin(), schedule_files_.end(),
        [&path](const ScheduleFile& file) { return file.path == path; });
    
    if (it != schedule_files_.end()) {
        schedule_files_.erase(it);
        save();
        LOG_INFO("Removed schedule file: " + path);
    }
}

void Config::update_schedule_file(const ScheduleFile& file) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(schedule_files_.begin(), schedule_files_.end(),
        [&file](const ScheduleFile& existing) { return existing.path == file.path; });
    
    if (it != schedule_files_.end()) {
        *it = file;
        save();
        LOG_INFO("Updated schedule file: " + file.path);
    }
}

bool Config::is_enabled() {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

void Config::set_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    save();
}

int Config::get_check_interval_seconds() {
    std::lock_guard<std::mutex> lock(mutex_);
    return check_interval_seconds_;
}

void Config::set_check_interval_seconds(int interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_interval_seconds_ = interval;
    save();
}

std::string Config::get_timezone() {
    std::lock_guard<std::mutex> lock(mutex_);
    return timezone_;
}

void Config::set_timezone(const std::string& timezone) {
    std::lock_guard<std::mutex> lock(mutex_);
    timezone_ = timezone;
    save();
}

bool Config::is_debug_mode() {
    std::lock_guard<std::mutex> lock(mutex_);
    return debug_mode_;
}

void Config::set_debug_mode(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    debug_mode_ = enabled;
    save();
}

void Config::load_default_config() {
    enabled_ = true;
    check_interval_seconds_ = 1;
    timezone_ = "UTC";
    debug_mode_ = false;
    schedule_files_.clear();
    
    // Add a default schedule file
    ScheduleFile default_file;
    default_file.path = get_default_schedule_path();
    default_file.enabled = true;
    default_file.name = "Default Schedule";
    schedule_files_.push_back(default_file);
}

std::string Config::escape_json_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default: result += c; break;
        }
    }
    return result;
}

std::string Config::unescape_json_string(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
            case '"': result += '"'; ++i; break;
            case '\\': result += '\\'; ++i; break;
            case 'b': result += '\b'; ++i; break;
            case 'f': result += '\f'; ++i; break;
            case 'n': result += '\n'; ++i; break;
            case 'r': result += '\r'; ++i; break;
            case 't': result += '\t'; ++i; break;
            default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}
