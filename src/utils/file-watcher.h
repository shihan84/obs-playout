#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/inotify.h>
#include <unistd.h>
#endif

class FileWatcher {
public:
    using FileChangeCallback = std::function<void(const std::string& file_path)>;
    
    FileWatcher();
    ~FileWatcher();
    
    bool initialize();
    void cleanup();
    
    // File monitoring
    bool add_file(const std::string& file_path, FileChangeCallback callback);
    bool remove_file(const std::string& file_path);
    void clear_all_files();
    
    // Control
    void start();
    void stop();
    bool is_running() const;
    
    // Status
    size_t get_watched_files_count() const;
    std::vector<std::string> get_watched_files() const;

private:
    struct WatchedFile {
        std::string path;
        FileChangeCallback callback;
#ifdef _WIN32
        HANDLE handle;
        OVERLAPPED overlapped;
        BYTE buffer[1024];
#else
        int wd;
#endif
    };
    
    void watcher_loop();
    bool setup_platform_watcher();
    void cleanup_platform_watcher();
    
    std::unique_ptr<std::thread> watcher_thread_;
    std::atomic<bool> running_;
    
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<WatchedFile>> watched_files_;
    
#ifdef _WIN32
    HANDLE completion_port_;
#else
    int inotify_fd_;
#endif
    
    // Prevent copying
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;
};
