#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

class Logger {
public:
    enum class Level {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

    static void initialize();
    static void cleanup();
    static void set_level(Level level);
    static void set_file_path(const std::string& path);
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    
    static void log(Level level, const std::string& message);

private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex mutex_;
    
    std::ofstream file_stream_;
    Level current_level_;
    std::string log_file_path_;
    std::mutex log_mutex_;
    
    Logger();
    ~Logger();
    
    void write_log(Level level, const std::string& message);
    std::string level_to_string(Level level) const;
    std::string get_timestamp() const;
    
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARNING(msg) Logger::warning(msg)
#define LOG_ERROR(msg) Logger::error(msg)
