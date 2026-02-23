#include "file-watcher.h"
#include "logger.h"
#include <filesystem>

FileWatcher::FileWatcher()
    : running_(false)
#ifdef _WIN32
    , completion_port_(INVALID_HANDLE_VALUE)
#else
    , inotify_fd_(-1)
#endif
{
}

FileWatcher::~FileWatcher() {
    cleanup();
}

bool FileWatcher::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Initializing file watcher");
    
    if (!setup_platform_watcher()) {
        LOG_ERROR("Failed to setup platform file watcher");
        return false;
    }
    
    LOG_INFO("File watcher initialized successfully");
    return true;
}

void FileWatcher::cleanup() {
    stop();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    cleanup_platform_watcher();
    watched_files_.clear();
    
    LOG_INFO("File watcher cleaned up");
}

bool FileWatcher::add_file(const std::string& file_path, FileChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!std::filesystem::exists(file_path)) {
        LOG_WARNING("File does not exist: " + file_path);
        return false;
    }
    
    // Remove existing watch if present
    remove_file(file_path);
    
    auto watched_file = std::make_unique<WatchedFile>();
    watched_file->path = file_path;
    watched_file->callback = callback;
    
#ifdef _WIN32
    std::wstring wide_path(file_path.begin(), file_path.end());
    watched_file->handle = CreateFileW(
        wide_path.c_str(),
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    
    if (watched_file->handle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to open file for watching: " + file_path);
        return false;
    }
    
    memset(&watched_file->overlapped, 0, sizeof(OVERLAPPED));
    watched_file->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (!ReadDirectoryChangesW(
        watched_file->handle,
        watched_file->buffer,
        sizeof(watched_file->buffer),
        FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL,
        &watched_file->overlapped,
        NULL)) {
        
        CloseHandle(watched_file->handle);
        CloseHandle(watched_file->overlapped.hEvent);
        LOG_ERROR("Failed to setup directory change notification: " + file_path);
        return false;
    }
    
    // Associate with completion port
    if (!CreateIoCompletionPort(watched_file->handle, completion_port_, (ULONG_PTR)watched_file.get(), 0)) {
        CloseHandle(watched_file->handle);
        CloseHandle(watched_file->overlapped.hEvent);
        LOG_ERROR("Failed to associate file handle with completion port: " + file_path);
        return false;
    }
    
#else
    std::string dir_path = std::filesystem::path(file_path).parent_path().string();
    std::string filename = std::filesystem::path(file_path).filename().string();
    
    watched_file->wd = inotify_add_watch(inotify_fd_, dir_path.c_str(), 
                                          IN_MODIFY | IN_MOVED_TO | IN_CREATE);
    
    if (watched_file->wd == -1) {
        LOG_ERROR("Failed to add inotify watch: " + file_path);
        return false;
    }
#endif
    
    watched_files_[file_path] = std::move(watched_file);
    
    LOG_INFO("Added file to watcher: " + file_path);
    return true;
}

bool FileWatcher::remove_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = watched_files_.find(file_path);
    if (it == watched_files_.end()) {
        return false;
    }
    
    auto& watched_file = it->second;
    
#ifdef _WIN32
    if (watched_file->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(watched_file->handle);
    }
    if (watched_file->overlapped.hEvent) {
        CloseHandle(watched_file->overlapped.hEvent);
    }
#else
    if (watched_file->wd != -1) {
        inotify_rm_watch(inotify_fd_, watched_file->wd);
    }
#endif
    
    watched_files_.erase(it);
    
    LOG_INFO("Removed file from watcher: " + file_path);
    return true;
}

void FileWatcher::clear_all_files() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : watched_files_) {
        auto& watched_file = pair.second;
        
#ifdef _WIN32
        if (watched_file->handle != INVALID_HANDLE_VALUE) {
            CloseHandle(watched_file->handle);
        }
        if (watched_file->overlapped.hEvent) {
            CloseHandle(watched_file->overlapped.hEvent);
        }
#else
        if (watched_file->wd != -1) {
            inotify_rm_watch(inotify_fd_, watched_file->wd);
        }
#endif
    }
    
    watched_files_.clear();
    
    LOG_INFO("Cleared all watched files");
}

void FileWatcher::start() {
    if (running_) {
        return;
    }
    
    LOG_INFO("Starting file watcher");
    running_ = true;
    
    watcher_thread_ = std::make_unique<std::thread>(&FileWatcher::watcher_loop, this);
    
    LOG_INFO("File watcher started");
}

void FileWatcher::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping file watcher");
    running_ = false;
    
    if (watcher_thread_ && watcher_thread_->joinable()) {
        watcher_thread_->join();
    }
    
    LOG_INFO("File watcher stopped");
}

bool FileWatcher::is_running() const {
    return running_;
}

size_t FileWatcher::get_watched_files_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return watched_files_.size();
}

std::vector<std::string> FileWatcher::get_watched_files() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    result.reserve(watched_files_.size());
    
    for (const auto& pair : watched_files_) {
        result.push_back(pair.first);
    }
    
    return result;
}

void FileWatcher::watcher_loop() {
    LOG_INFO("File watcher loop started");
    
    while (running_) {
        try {
#ifdef _WIN32
            DWORD bytes_transferred;
            ULONG_PTR completion_key;
            LPOVERLAPPED overlapped;
            
            BOOL result = GetQueuedCompletionStatus(
                completion_port_,
                &bytes_transferred,
                &completion_key,
                &overlapped,
                1000  // 1 second timeout
            );
            
            if (result && overlapped && bytes_transferred > 0) {
                WatchedFile* watched_file = reinterpret_cast<WatchedFile*>(completion_key);
                
                if (watched_file && watched_file->callback) {
                    // Check if this is a notification for our file
                    FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(watched_file->buffer);
                    
                    while (true) {
                        if (info->Action == FILE_ACTION_MODIFIED || 
                            info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                            
                            std::lock_guard<std::mutex> lock(mutex_);
                            watched_file->callback(watched_file->path);
                            break;
                        }
                        
                        if (info->NextEntryOffset == 0) {
                            break;
                        }
                        
                        info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                            reinterpret_cast<BYTE*>(info) + info->NextEntryOffset);
                    }
                }
                
                // Restart the read operation
                ReadDirectoryChangesW(
                    watched_file->handle,
                    watched_file->buffer,
                    sizeof(watched_file->buffer),
                    FALSE,
                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
                    NULL,
                    &watched_file->overlapped,
                    NULL
                );
            }
#else
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(inotify_fd_, &read_fds);
            
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int result = select(inotify_fd_ + 1, &read_fds, NULL, NULL, &timeout);
            
            if (result > 0 && FD_ISSET(inotify_fd_, &read_fds)) {
                char buffer[4096];
                ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
                
                if (length > 0) {
                    size_t i = 0;
                    while (i < static_cast<size_t>(length)) {
                        struct inotify_event* event = reinterpret_cast<struct inotify_event*>(buffer + i);
                        
                        if (event->mask & (IN_MODIFY | IN_MOVED_TO | IN_CREATE)) {
                            std::string changed_file;
                            
                            std::lock_guard<std::mutex> lock(mutex_);
                            for (const auto& pair : watched_files_) {
                                const auto& watched_file = pair.second;
                                if (watched_file->wd == event->wd) {
                                    std::string filename(event->name);
                                    std::string watched_filename = std::filesystem::path(pair.first).filename().string();
                                    
                                    if (filename == watched_filename) {
                                        watched_file->callback(pair.first);
                                        break;
                                    }
                                }
                            }
                        }
                        
                        i += sizeof(struct inotify_event) + event->len;
                    }
                }
            }
#endif
            
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in file watcher loop: " + std::string(e.what()));
        }
    }
    
    LOG_INFO("File watcher loop ended");
}

bool FileWatcher::setup_platform_watcher() {
#ifdef _WIN32
    completion_port_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (completion_port_ == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to create IO completion port");
        return false;
    }
    
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    SetThreadpoolThreadMaximum(&sys_info);
    
#else
    inotify_fd_ = inotify_init();
    if (inotify_fd_ == -1) {
        LOG_ERROR("Failed to initialize inotify");
        return false;
    }
#endif
    
    return true;
}

void FileWatcher::cleanup_platform_watcher() {
#ifdef _WIN32
    if (completion_port_ != INVALID_HANDLE_VALUE) {
        CloseHandle(completion_port_);
        completion_port_ = INVALID_HANDLE_VALUE;
    }
#else
    if (inotify_fd_ != -1) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
#endif
}
