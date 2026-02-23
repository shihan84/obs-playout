#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

struct ScheduledItem;

class MediaController {
public:
    MediaController();
    ~MediaController();
    
    bool initialize();
    void cleanup();
    
    // Media source control
    bool play_media(const std::string& source_name, const std::string& file_path = "");
    bool stop_media(const std::string& source_name);
    bool restart_media(const std::string& source_name);
    bool pause_media(const std::string& source_name);
    bool resume_media(const std::string& source_name);
    
    // Scene control
    bool switch_to_scene(const std::string& scene_name);
    std::string get_current_scene() const;
    
    // Source visibility
    bool set_source_visibility(const std::string& source_name, bool visible);
    bool get_source_visibility(const std::string& source_name) const;
    
    // Scheduled item execution
    bool execute_item(const ScheduledItem& item);
    bool play_idle_content();
    
    // Source discovery
    std::vector<std::string> get_media_sources() const;
    std::vector<std::string> get_scenes() const;
    std::vector<std::string> get_sources_in_scene(const std::string& scene_name) const;
    
    // Media state monitoring
    std::string get_media_state(const std::string& source_name) const;
    int get_media_duration(const std::string& source_name) const;
    int get_media_time(const std::string& source_name) const;
    bool is_media_ended(const std::string& source_name) const;
    
    // Callbacks for media events
    using MediaEventCallback = std::function<void(const std::string& source_name, const std::string& event)>;
    void set_media_event_callback(MediaEventCallback callback);
    
    // Validation
    bool validate_media_source(const std::string& source_name) const;
    bool validate_scene(const std::string& scene_name) const;
    bool validate_file_path(const std::string& file_path) const;

private:
    mutable std::mutex mutex_;
    MediaEventCallback media_event_callback_;
    
    // OBS source references
    std::map<std::string, obs_source_t*> media_sources_;
    std::map<std::string, obs_scene_t*> scenes_;
    
    // Configuration
    std::string default_idle_content_;
    std::vector<Config::ScheduleFile> auto_schedule_files_;
    bool auto_switch_scenes_;
    bool fade_transitions_;
    int transition_duration_ms_;
    
    // Internal methods
    obs_source_t* get_media_source(const std::string& source_name);
    obs_scene_t* get_scene(const std::string& scene_name);
    obs_sceneitem_t* get_scene_item(obs_scene_t* scene, const std::string& source_name);
    
    void refresh_source_list();
    void refresh_scene_list();
    
    // Media control helpers
    bool set_media_file(obs_source_t* source, const std::string& file_path);
    bool trigger_media_action(obs_source_t* source, const std::string& action);
    
    // Transition helpers
    bool execute_scene_transition(const std::string& from_scene, const std::string& to_scene);
    void fade_source_visibility(const std::string& source_name, bool visible, int duration_ms);
    
    // Event handlers
    static void media_source_callback(void* data, calldata_t* cd);
    void handle_media_event(const std::string& source_name, const std::string& event);
    
    // Utility methods
    std::string get_obs_error_string(obs_error_t error) const;
    void log_obs_error(const std::string& operation, obs_error_t error) const;
    
    // Prevent copying
    MediaController(const MediaController&) = delete;
    MediaController& operator=(const MediaController&) = delete;
};
