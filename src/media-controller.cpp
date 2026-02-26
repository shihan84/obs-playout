#include "media-controller.h"
#include "playlist-manager.h"
#include "utils/config.h"
#include "utils/logger.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/dstr.h>
#include <filesystem>

MediaController::MediaController()
    : auto_switch_scenes_(true)
    , fade_transitions_(true)
    , transition_duration_ms_(500)
{
}

MediaController::~MediaController() {
    cleanup();
}

bool MediaController::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Initializing media controller");
    
    try {
        // Load configuration
        auto_schedule_files_ = Config::get_schedule_files();
        if (!auto_schedule_files_.empty()) {
            default_idle_content_ = auto_schedule_files_[0].path;
        }
        
        // Refresh source and scene lists
        refresh_source_list();
        refresh_scene_list();
        
        LOG_INFO("Media controller initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during media controller initialization: " + std::string(e.what()));
        return false;
    }
}

void MediaController::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Release source references
    for (auto& pair : media_sources_) {
        if (pair.second) {
            obs_source_release(pair.second);
        }
    }
    media_sources_.clear();
    
    // Release scene references
    for (auto& pair : scenes_) {
        if (pair.second) {
            obs_scene_release(pair.second);
        }
    }
    scenes_.clear();
    
    media_event_callback_ = nullptr;
    
    LOG_INFO("Media controller cleaned up");
}

bool MediaController::play_media(const std::string& source_name, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        obs_source_t* source = get_media_source(source_name);
        if (!source) {
            LOG_ERROR("Media source not found: " + source_name);
            return false;
        }
        
        // Set file if provided
        if (!file_path.empty()) {
            if (!set_media_file(source, file_path)) {
                LOG_ERROR("Failed to set media file: " + file_path);
                return false;
            }
        }
        
        // Start playback
        obs_source_media_play_pause(source, false);
        
        LOG_INFO("Started playback on source: " + source_name);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception playing media on " + source_name + ": " + std::string(e.what()));
        return false;
    }
}

bool MediaController::stop_media(const std::string& source_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        obs_source_t* source = get_media_source(source_name);
        if (!source) {
            LOG_ERROR("Media source not found: " + source_name);
            return false;
        }
        
        obs_source_media_stop(source);
        
        LOG_INFO("Stopped playback on source: " + source_name);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception stopping media on " + source_name + ": " + std::string(e.what()));
        return false;
    }
}

bool MediaController::restart_media(const std::string& source_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        obs_source_t* source = get_media_source(source_name);
        if (!source) {
            LOG_ERROR("Media source not found: " + source_name);
            return false;
        }
        
        obs_source_media_restart(source);
        
        LOG_INFO("Restarted playback on source: " + source_name);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception restarting media on " + source_name + ": " + std::string(e.what()));
        return false;
    }
}

bool MediaController::pause_media(const std::string& source_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        obs_source_t* source = get_media_source(source_name);
        if (!source) {
            LOG_ERROR("Media source not found: " + source_name);
            return false;
        }
        
        obs_source_media_play_pause(source, true);
        
        LOG_INFO("Paused playback on source: " + source_name);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception pausing media on " + source_name + ": " + std::string(e.what()));
        return false;
    }
}

bool MediaController::resume_media(const std::string& source_name) {
    return play_media(source_name);
}

bool MediaController::switch_to_scene(const std::string& scene_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        obs_scene_t* scene = get_scene(scene_name);
        if (!scene) {
            LOG_ERROR("Scene not found: " + scene_name);
            return false;
        }
        
        obs_source_t* scene_source = obs_scene_get_source(scene);
        if (!scene_source) {
            LOG_ERROR("Failed to get scene source for: " + scene_name);
            return false;
        }
        
        // Switch to the scene
        obs_frontend_set_current_scene(scene_source);
        
        LOG_INFO("Switched to scene: " + scene_name);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception switching to scene " + scene_name + ": " + std::string(e.what()));
        return false;
    }
}

std::string MediaController::get_current_scene() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    obs_source_t* current_scene = obs_frontend_get_current_scene();
    if (!current_scene) {
        return "";
    }
    
    const char* scene_name = obs_source_get_name(current_scene);
    std::string result = scene_name ? scene_name : "";
    
    obs_source_release(current_scene);
    return result;
}

bool MediaController::set_source_visibility(const std::string& source_name, bool visible) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Get current scene
        obs_source_t* current_scene_source = obs_frontend_get_current_scene();
        if (!current_scene_source) {
            LOG_ERROR("Failed to get current scene");
            return false;
        }
        
        obs_scene_t* current_scene = obs_scene_from_source(current_scene_source);
        if (!current_scene) {
            obs_source_release(current_scene_source);
            LOG_ERROR("Failed to get current scene from source");
            return false;
        }
        
        // Find the scene item
        obs_sceneitem_t* item = get_scene_item(current_scene, source_name);
        if (!item) {
            obs_scene_release(current_scene);
            obs_source_release(current_scene_source);
            LOG_ERROR("Source not found in current scene: " + source_name);
            return false;
        }
        
        // Set visibility
        obs_sceneitem_set_visible(item, visible);
        
        // Cleanup
        obs_sceneitem_release(item);
        obs_scene_release(current_scene);
        obs_source_release(current_scene_source);
        
        LOG_INFO("Set source visibility: " + source_name + " -> " + (visible ? "visible" : "hidden"));
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception setting source visibility for " + source_name + ": " + std::string(e.what()));
        return false;
    }
}

bool MediaController::get_source_visibility(const std::string& source_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    obs_source_t* current_scene_source = obs_frontend_get_current_scene();
    if (!current_scene_source) {
        return false;
    }
    
    obs_scene_t* current_scene = obs_scene_from_source(current_scene_source);
    if (!current_scene) {
        obs_source_release(current_scene_source);
        return false;
    }
    
    obs_sceneitem_t* item = get_scene_item(current_scene, source_name);
    if (!item) {
        obs_scene_release(current_scene);
        obs_source_release(current_scene_source);
        return false;
    }
    
    bool visible = obs_sceneitem_visible(item);
    
    obs_sceneitem_release(item);
    obs_scene_release(current_scene);
    obs_source_release(current_scene_source);
    
    return visible;
}

bool MediaController::execute_item(const ScheduledItem& item) {
    LOG_INFO("Executing scheduled item: " + item.name);
    
    try {
        // Switch scene if specified
        if (!item.scene.empty()) {
            if (!switch_to_scene(item.scene)) {
                LOG_WARNING("Failed to switch to scene: " + item.scene);
            }
        }
        
        // Set source visibility if needed
        if (!item.source.empty()) {
            // First, make sure the source is visible
            if (!set_source_visibility(item.source, true)) {
                LOG_WARNING("Failed to set source visibility: " + item.source);
            }
        }
        
        // Play the media
        if (!item.file_path.empty()) {
            if (!play_media(item.source, item.file_path)) {
                LOG_ERROR("Failed to play media: " + item.file_path);
                return false;
            }
        } else {
            if (!play_media(item.source)) {
                LOG_ERROR("Failed to play media source: " + item.source);
                return false;
            }
        }
        
        LOG_INFO("Successfully executed scheduled item: " + item.name);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception executing scheduled item " + item.name + ": " + std::string(e.what()));
        return false;
    }
}

bool MediaController::play_idle_content() {
    if (default_idle_content_.empty()) {
        LOG_WARNING("No default idle content configured");
        return false;
    }
    
    LOG_INFO("Playing idle content: " + default_idle_content_);
    
    // Try to find a media source to play idle content
    auto media_sources = get_media_sources();
    if (!media_sources.empty()) {
        return play_media(media_sources[0], default_idle_content_);
    }
    
    LOG_ERROR("No media sources available for idle content");
    return false;
}

std::vector<std::string> MediaController::get_media_sources() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    
    // Iterate through all sources and find media sources
    obs_enum_sources([](void* data, obs_source_t* source) {
        auto* sources = static_cast<std::vector<std::string>*>(data);
        
        const char* source_id = obs_source_get_id(source);
        if (source_id && (strcmp(source_id, "ffmpeg_source") == 0 || 
                          strcmp(source_id, "media_source") == 0 ||
                          strcmp(source_id, "vlc_source") == 0)) {
            
            const char* name = obs_source_get_name(source);
            if (name) {
                sources->push_back(std::string(name));
            }
        }
        
        return true;
    }, &result);
    
    return result;
}

std::vector<std::string> MediaController::get_scenes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    
    obs_enum_scenes([](void* data, obs_source_t* scene) {
        auto* scenes = static_cast<std::vector<std::string>*>(data);
        
        const char* name = obs_source_get_name(scene);
        if (name) {
            scenes->push_back(std::string(name));
        }
        
        return true;
    }, &result);
    
    return result;
}

std::vector<std::string> MediaController::get_sources_in_scene(const std::string& scene_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    
    obs_scene_t* scene = get_scene(scene_name);
    if (!scene) {
        return result;
    }
    
    obs_scene_enum_items(scene, [](obs_scene_t*, obs_sceneitem_t* item, void* data) {
        auto* sources = static_cast<std::vector<std::string>*>(data);
        
        obs_source_t* source = obs_sceneitem_get_source(item);
        if (source) {
            const char* name = obs_source_get_name(source);
            if (name) {
                sources->push_back(std::string(name));
            }
        }
        
        return true;
    }, &result);
    
    obs_scene_release(scene);
    return result;
}

std::string MediaController::get_media_state(const std::string& source_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    obs_source_t* source = get_media_source(source_name);
    if (!source) {
        return "unknown";
    }
    
    obs_media_state_t state = obs_source_media_get_state(source);
    
    switch (state) {
    case OBS_MEDIA_STATE_NONE:      return "none";
    case OBS_MEDIA_STATE_PLAYING:   return "playing";
    case OBS_MEDIA_STATE_PAUSED:    return "paused";
    case OBS_MEDIA_STATE_STOPPED:   return "stopped";
    case OBS_MEDIA_STATE_ENDED:     return "ended";
    case OBS_MEDIA_STATE_ERROR:     return "error";
    default:                        return "unknown";
    }
    
    // This should never be reached due to the default case above
    return "unknown";
}

int MediaController::get_media_duration(const std::string& source_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    obs_source_t* source = get_media_source(source_name);
    if (!source) {
        return 0;
    }
    
    return static_cast<int>(obs_source_media_get_duration(source) / 1000000); // Convert from ns to ms
}

int MediaController::get_media_time(const std::string& source_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    obs_source_t* source = get_media_source(source_name);
    if (!source) {
        return 0;
    }
    
    return static_cast<int>(obs_source_media_get_time(source) / 1000000); // Convert from ns to ms
}

bool MediaController::is_media_ended(const std::string& source_name) const {
    return get_media_state(source_name) == "ended";
}

void MediaController::set_media_event_callback(MediaEventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    media_event_callback_ = callback;
}

bool MediaController::validate_media_source(const std::string& source_name) const {
    obs_source_t* source = get_media_source(source_name);
    return source != nullptr;
}

bool MediaController::validate_scene(const std::string& scene_name) const {
    obs_scene_t* scene = get_scene(scene_name);
    if (scene) {
        obs_scene_release(scene);
        return true;
    }
    return false;
}

bool MediaController::validate_file_path(const std::string& file_path) const {
    return std::filesystem::exists(file_path);
}

// Private methods implementation

obs_source_t* MediaController::get_media_source(const std::string& source_name) {
    auto it = media_sources_.find(source_name);
    if (it != media_sources_.end() && it->second) {
        return it->second;
    }
    
    // Try to find the source
    obs_source_t* source = obs_get_source_by_name(source_name.c_str());
    if (source) {
        // Check if it's a media source
        const char* source_id = obs_source_get_id(source);
        if (source_id && (strcmp(source_id, "ffmpeg_source") == 0 || 
                          strcmp(source_id, "media_source") == 0 ||
                          strcmp(source_id, "vlc_source") == 0)) {
            media_sources_[source_name] = source;
            return source;
        }
        
        obs_source_release(source);
    }
    
    return nullptr;
}

obs_scene_t* MediaController::get_scene(const std::string& scene_name) {
    auto it = scenes_.find(scene_name);
    if (it != scenes_.end() && it->second) {
        return it->second;
    }
    
    obs_source_t* scene_source = obs_get_source_by_name(scene_name.c_str());
    if (scene_source) {
        obs_scene_t* scene = obs_scene_from_source(scene_source);
        if (scene) {
            scenes_[scene_name] = scene;
            obs_source_release(scene_source);
            return scene;
        }
        obs_source_release(scene_source);
    }
    
    return nullptr;
}

obs_sceneitem_t* MediaController::get_scene_item(obs_scene_t* scene, const std::string& source_name) {
    obs_sceneitem_t* item = nullptr;
    
    obs_scene_enum_items(scene, [](obs_scene_t*, obs_sceneitem_t* current_item, void* data) {
        auto* search_data = static_cast<std::pair<std::string, obs_sceneitem_t*>*>(data);
        
        obs_source_t* source = obs_sceneitem_get_source(current_item);
        if (source) {
            const char* name = obs_source_get_name(source);
            if (name && std::string(name) == search_data->first) {
                search_data->second = current_item;
                obs_sceneitem_addref(current_item);
                return false; // Stop enumeration
            }
        }
        
        return true; // Continue enumeration
    }, &std::make_pair(source_name, item));
    
    return item;
}

void MediaController::refresh_source_list() {
    // Clear existing references
    for (auto& pair : media_sources_) {
        if (pair.second) {
            obs_source_release(pair.second);
        }
    }
    media_sources_.clear();
    
    // This will be repopulated on demand
}

void MediaController::refresh_scene_list() {
    // Clear existing references
    for (auto& pair : scenes_) {
        if (pair.second) {
            obs_scene_release(pair.second);
        }
    }
    scenes_.clear();
    
    // This will be repopulated on demand
}

bool MediaController::set_media_file(obs_source_t* source, const std::string& file_path) {
    if (!source) {
        return false;
    }
    
    // Get source settings
    obs_data_t* settings = obs_source_get_settings(source);
    if (!settings) {
        return false;
    }
    
    // Set the file path
    obs_data_set_string(settings, "file", file_path.c_str());
    
    // Apply the settings
    obs_source_update(source, settings);
    
    // Cleanup
    obs_data_release(settings);
    
    return true;
}

void MediaController::handle_media_event(const std::string& source_name, const std::string& event) {
    if (media_event_callback_) {
        media_event_callback_(source_name, event);
    }
}
