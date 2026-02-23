#include "logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

std::unique_ptr<Logger> Logger::instance_;
std::mutex Logger::mutex_;

Logger::Logger() : current_level_(Level::INFO) {
    // Set default log file path
#ifdef _WIN32
    char app_data[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, app_data) == S_OK) {
        log_file_path_ = std::string(app_data) + "\\obs-time-scheduler\\logs\\scheduler.log";
    } else {
        log_file_path_ = "scheduler.log";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        log_file_path_ = std::string(home) + "/.config/obs-time-scheduler/logs/scheduler.log";
    } else {
        log_file_path_ = "scheduler.log";
    }
#endif
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
}

void Logger::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    instance_.reset();
}

void Logger::set_level(Level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
        instance_->current_level_ = level;
    }
}

void Logger::set_file_path(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
        instance_->log_file_path_ = path;
        // Close current file and reopen with new path
        if (instance_->file_stream_.is_open()) {
            instance_->file_stream_.close();
        }
    }
}

void Logger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(Level::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void Logger::log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ && level >= instance_->current_level_) {
        instance_->write_log(level, message);
    }
    
    // Also log to OBS blog
    switch (level) {
    case Level::DEBUG:
        blog(LOG_DEBUG, "[Time Scheduler] %s", message.c_str());
        break;
    case Level::INFO:
        blog(LOG_INFO, "[Time Scheduler] %s", message.c_str());
        break;
    case Level::WARNING:
        blog(LOG_WARNING, "[Time Scheduler] %s", message.c_str());
        break;
    case Level::ERROR:
        blog(LOG_ERROR, "[Time Scheduler] %s", message.c_str());
        break;
    }
}

void Logger::write_log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> file_lock(log_mutex_);
    
    // Open file if not already open
    if (!file_stream_.is_open()) {
        // Create directory if it doesn't exist
        size_t last_slash = log_file_path_.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            std::string dir = log_file_path_.substr(0, last_slash);
#ifdef _WIN32
            CreateDirectoryA(dir.c_str(), NULL);
#else
            mkdir(dir.c_str(), 0755);
#endif
        }
        
        file_stream_.open(log_file_path_, std::ios::app);
        if (!file_stream_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file_path_ << std::endl;
            return;
        }
    }
    
    // Write log entry
    file_stream_ << get_timestamp() << " [" << level_to_string(level) << "] " << message << std::endl;
    file_stream_.flush();
}

std::string Logger::level_to_string(Level level) const {
    switch (level) {
    case Level::DEBUG:   return "DEBUG";
    case Level::INFO:    return "INFO";
    case Level::WARNING: return "WARNING";
    case Level::ERROR:   return "ERROR";
    default:             return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}
