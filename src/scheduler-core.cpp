#include "scheduler-core.h"
#include "playlist-manager.h"
#include "media-controller.h"
#include "time-trigger.h"
#include "utils/config.h"
#include "utils/logger.h"
#include <chrono>

SchedulerCore::SchedulerCore()
    : running_(false)
    , enabled_(true)
    , should_reload_(false)
    , check_interval_seconds_(1)
    , last_check_time_(std::chrono::steady_clock::now())
{
}

SchedulerCore::~SchedulerCore() {
    stop();
}

bool SchedulerCore::initialize() {
    LOG_INFO("Initializing scheduler core");
    
    try {
        // Initialize components
        playlist_manager_ = std::make_unique<PlaylistManager>();
        if (!playlist_manager_->initialize()) {
            LOG_ERROR("Failed to initialize playlist manager");
            return false;
        }
        
        media_controller_ = std::make_unique<MediaController>();
        if (!media_controller_->initialize()) {
            LOG_ERROR("Failed to initialize media controller");
            return false;
        }
        
        time_trigger_ = std::make_unique<TimeTrigger>();
        if (!time_trigger_->initialize()) {
            LOG_ERROR("Failed to initialize time trigger");
            return false;
        }
        
        // Load configuration
        enabled_ = Config::is_enabled();
        check_interval_seconds_ = Config::get_check_interval_seconds();
        
        LOG_INFO("Scheduler core initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during scheduler initialization: " + std::string(e.what()));
        return false;
    }
}

void SchedulerCore::start() {
    if (running_) {
        LOG_WARNING("Scheduler is already running");
        return;
    }
    
    LOG_INFO("Starting scheduler");
    running_ = true;
    
    // Start scheduler thread
    scheduler_thread_ = std::make_unique<std::thread>(&SchedulerCore::scheduler_loop, this);
    
    LOG_INFO("Scheduler started successfully");
}

void SchedulerCore::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping scheduler");
    running_ = false;
    
    // Wake up the scheduler thread
    cv_.notify_all();
    
    // Wait for thread to finish
    if (scheduler_thread_ && scheduler_thread_->joinable()) {
        scheduler_thread_->join();
    }
    
    LOG_INFO("Scheduler stopped");
}

void SchedulerCore::toggle_enabled() {
    enabled_ = !enabled_;
    Config::set_enabled(enabled_);
    
    LOG_INFO("Scheduler " + std::string(enabled_ ? "enabled" : "disabled"));
    
    // Wake up scheduler thread to apply change
    cv_.notify_all();
}

bool SchedulerCore::is_running() const {
    return running_;
}

bool SchedulerCore::is_enabled() const {
    return enabled_;
}

void SchedulerCore::reload_schedules() {
    should_reload_ = true;
    cv_.notify_all();
    LOG_INFO("Schedule reload requested");
}

void SchedulerCore::force_check() {
    cv_.notify_all();
    LOG_DEBUG("Force check requested");
}

std::string SchedulerCore::get_status() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    if (!running_) {
        return "Stopped";
    }
    
    if (!enabled_) {
        return "Disabled";
    }
    
    return "Running";
}

std::string SchedulerCore::get_current_item() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return current_item_id_;
}

std::string SchedulerCore::get_next_item() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return next_item_id_;
}

void SchedulerCore::scheduler_loop() {
    LOG_INFO("Scheduler loop started");
    
    while (running_) {
        try {
            if (enabled_) {
                // Check if we need to reload schedules
                if (should_reload_) {
                    should_reload_ = false;
                    playlist_manager_->reload_schedules();
                    LOG_INFO("Schedules reloaded");
                }
                
                // Check and execute scheduled items
                check_and_execute_schedules();
            }
            
            // Wait for next check interval or notification
            std::unique_lock<std::mutex> lock(cv_mutex_);
            cv_.wait_for(lock, std::chrono::seconds(check_interval_seconds_), 
                        [this] { return !running_ || should_reload_; });
            
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in scheduler loop: " + std::string(e.what()));
            // Continue running even if there's an error
        }
    }
    
    LOG_INFO("Scheduler loop ended");
}

void SchedulerCore::check_and_execute_schedules() {
    auto now = std::chrono::steady_clock::now();
    
    // Check if enough time has passed since last check
    auto time_since_last_check = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_check_time_);
    
    if (time_since_last_check.count() < check_interval_seconds_) {
        return;
    }
    
    last_check_time_ = now;
    
    try {
        // Get current time-based items that should execute
        auto current_items = time_trigger_->get_current_items();
        auto next_items = time_trigger_->get_next_items();
        
        // Update status
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            next_item_id_ = next_items.empty() ? "None" : next_items[0];
        }
        
        // Execute current items
        for (const auto& item_id : current_items) {
            // Check if this item is already playing
            std::lock_guard<std::mutex> lock(status_mutex_);
            if (current_item_id_ != item_id) {
                execute_scheduled_item(item_id);
                current_item_id_ = item_id;
            }
        }
        
        // Handle idle state if no current items
        if (current_items.empty()) {
            std::lock_guard<std::mutex> lock(status_mutex_);
            if (!current_item_id_.empty() && current_item_id_ != "idle") {
                execute_scheduled_item("idle");
                current_item_id_ = "idle";
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during schedule check: " + std::string(e.what()));
    }
}

void SchedulerCore::execute_scheduled_item(const std::string& item_id) {
    try {
        LOG_INFO("Executing scheduled item: " + item_id);
        
        if (item_id == "idle") {
            // Play default idle content
            media_controller_->play_idle_content();
            return;
        }
        
        // Get item details from playlist manager
        auto item = playlist_manager_->get_item(item_id);
        if (!item) {
            LOG_WARNING("Scheduled item not found: " + item_id);
            return;
        }
        
        // Execute media control actions
        media_controller_->execute_item(*item);
        
        LOG_INFO("Successfully executed scheduled item: " + item_id);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to execute scheduled item " + item_id + ": " + std::string(e.what()));
    }
}
