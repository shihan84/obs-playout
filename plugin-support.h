#pragma once

// Plugin version and metadata
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR "OBS Time Scheduler Team"
#define PLUGIN_WEBSITE "https://github.com/your-repo/obs-time-scheduler"

// Plugin configuration
#define PLUGIN_NAME "Time Scheduler"
#define PLUGIN_DESCRIPTION "Time-based playlist scheduler for OBS Studio"

// Build configuration
#ifdef _WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

// Feature flags
#define ENABLE_FILE_WATCHING 1
#define ENABLE_UI_SETTINGS 1
#define ENABLE_SCHEDULE_EDITOR 1
#define ENABLE_DEBUG_LOGGING 1

// Default configuration values
#define DEFAULT_CHECK_INTERVAL_SECONDS 1
#define DEFAULT_TRANSITION_DURATION_MS 500
#define DEFAULT_TIMEZONE "UTC"
#define MAX_SCHEDULE_FILES 10
#define MAX_ITEMS_PER_PLAYLIST 100

// File paths and extensions
#define SCHEDULE_FILE_EXTENSION ".json"
#define CONFIG_FILE_NAME "config.json"
#define LOG_FILE_NAME "scheduler.log"

// Time constants
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define SECONDS_PER_HOUR (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
#define SECONDS_PER_DAY (SECONDS_PER_HOUR * HOURS_PER_DAY)

// Error codes
enum class PluginError {
    SUCCESS = 0,
    INITIALIZATION_FAILED,
    CONFIG_LOAD_FAILED,
    SCHEDULE_PARSE_FAILED,
    OBS_CONNECTION_FAILED,
    MEDIA_SOURCE_NOT_FOUND,
    SCENE_NOT_FOUND,
    FILE_NOT_FOUND,
    INVALID_TIME_FORMAT,
    INVALID_JSON_FORMAT,
    PERMISSION_DENIED,
    UNKNOWN_ERROR
};

// Utility macros
#define PLUGIN_LOG(level, format, ...) \
    blog(level, "[Time Scheduler] " format, ##__VA_ARGS__)

#define PLUGIN_ERROR(format, ...) \
    PLUGIN_LOG(LOG_ERROR, format, ##__VA_ARGS__)

#define PLUGIN_WARNING(format, ...) \
    PLUGIN_LOG(LOG_WARNING, format, ##__VA_ARGS__)

#define PLUGIN_INFO(format, ...) \
    PLUGIN_LOG(LOG_INFO, format, ##__VA_ARGS__)

#define PLUGIN_DEBUG(format, ...) \
    PLUGIN_LOG(LOG_DEBUG, format, ##__VA_ARGS__)

// String conversion utilities
#ifdef _WIN32
#define PLUGIN_UTF8_TO_WIDE(s) L ## s
#define PLUGIN_WIDE_TO_UTF8(ws) std::string(ws.begin(), ws.end())
#else
#define PLUGIN_UTF8_TO_WIDE(s) s
#define PLUGIN_WIDE_TO_UTF8(ws) ws
#endif

// Memory management helpers
#define PLUGIN_SAFE_DELETE(ptr) \
    do { \
        if (ptr) { \
            delete ptr; \
            ptr = nullptr; \
        } \
    } while(0)

#define PLUGIN_SAFE_DELETE_ARRAY(ptr) \
    do { \
        if (ptr) { \
            delete[] ptr; \
            ptr = nullptr; \
        } \
    } while(0)

// Thread safety helpers
#define PLUGIN_LOCK_GUARD(mutex) std::lock_guard<std::mutex> lock(mutex)

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define PLUGIN_PLATFORM_WINDOWS
#elif defined(__linux__)
#define PLUGIN_PLATFORM_LINUX
#elif defined(__APPLE__)
#define PLUGIN_PLATFORM_MACOS
#endif

// Compiler-specific attributes
#ifdef __GNUC__
#define PLUGIN_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define PLUGIN_DEPRECATED __declspec(deprecated)
#else
#define PLUGIN_DEPRECATED
#endif

// Inline helper functions
namespace PluginUtils {
    inline std::string GetErrorString(PluginError error) {
        switch (error) {
        case PluginError::SUCCESS: return "Success";
        case PluginError::INITIALIZATION_FAILED: return "Initialization failed";
        case PluginError::CONFIG_LOAD_FAILED: return "Configuration load failed";
        case PluginError::SCHEDULE_PARSE_FAILED: return "Schedule parse failed";
        case PluginError::OBS_CONNECTION_FAILED: return "OBS connection failed";
        case PluginError::MEDIA_SOURCE_NOT_FOUND: return "Media source not found";
        case PluginError::SCENE_NOT_FOUND: return "Scene not found";
        case PluginError::FILE_NOT_FOUND: return "File not found";
        case PluginError::INVALID_TIME_FORMAT: return "Invalid time format";
        case PluginError::INVALID_JSON_FORMAT: return "Invalid JSON format";
        case PluginError::PERMISSION_DENIED: return "Permission denied";
        default: return "Unknown error";
        }
    }
    
    inline bool IsValidTimeString(const std::string& time_str) {
        if (time_str.length() != 5) return false;
        if (time_str[2] != ':') return false;
        
        int hour, minute;
        if (sscanf(time_str.c_str(), "%d:%d", &hour, &minute) != 2) return false;
        
        return (hour >= 0 && hour <= 23) && (minute >= 0 && minute <= 59);
    }
    
    inline int TimeStringToMinutes(const std::string& time_str) {
        int hour, minute;
        sscanf(time_str.c_str(), "%d:%d", &hour, &minute);
        return hour * 60 + minute;
    }
    
    inline std::string MinutesToTimeString(int minutes) {
        int hour = minutes / 60;
        int minute = minutes % 60;
        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
        return std::string(buffer);
    }
}
