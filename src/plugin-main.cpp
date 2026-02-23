#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include "utils/logger.h"
#include "utils/config.h"
#include "scheduler-core.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-time-scheduler", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Time-based playlist scheduler for OBS Studio";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Time Scheduler";
}

static obs_hotkey_id toggle_scheduler_hotkey = OBS_INVALID_HOTKEY_ID;
static SchedulerCore *scheduler = nullptr;

static void toggle_scheduler_hotkey_callback(void *data, obs_hotkey_id id,
					      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (pressed && scheduler) {
		scheduler->toggle_enabled();
	}
}

static void obs_frontend_event_callback(enum obs_frontend_event event, void *private_data)
{
	UNUSED_PARAMETER(private_data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		blog(LOG_INFO, "[Time Scheduler] OBS finished loading, initializing scheduler");
		if (scheduler) {
			scheduler->start();
		}
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		blog(LOG_INFO, "[Time Scheduler] OBS exiting, stopping scheduler");
		if (scheduler) {
			scheduler->stop();
		}
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		// Handle scene changes if needed
		break;
	default:
		break;
	}
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Time Scheduler] Loading plugin v%s", PLUGIN_VERSION);

	// Initialize logger
	Logger::initialize();

	// Load configuration
	Config::load();

	// Create scheduler instance
	scheduler = new SchedulerCore();
	if (!scheduler->initialize()) {
		blog(LOG_ERROR, "[Time Scheduler] Failed to initialize scheduler core");
		delete scheduler;
		scheduler = nullptr;
		return false;
	}

	// Register hotkey
	toggle_scheduler_hotkey = obs_hotkey_register_frontend(
		"obs_time_scheduler.toggle",
		"Toggle Time Scheduler",
		toggle_scheduler_hotkey_callback,
		nullptr);

	if (toggle_scheduler_hotkey == OBS_INVALID_HOTKEY_ID) {
		blog(LOG_WARNING, "[Time Scheduler] Failed to register toggle hotkey");
	}

	// Register frontend event callback
	obs_frontend_add_event_callback(obs_frontend_event_callback, nullptr);

	blog(LOG_INFO, "[Time Scheduler] Plugin loaded successfully");
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[Time Scheduler] Unloading plugin");

	// Unregister hotkey
	if (toggle_scheduler_hotkey != OBS_INVALID_HOTKEY_ID) {
		obs_hotkey_unregister(toggle_scheduler_hotkey);
	}

	// Remove frontend event callback
	obs_frontend_remove_event_callback(obs_frontend_event_callback, nullptr);

	// Cleanup scheduler
	if (scheduler) {
		scheduler->stop();
		delete scheduler;
		scheduler = nullptr;
	}

	// Save configuration
	Config::save();

	// Cleanup logger
	Logger::cleanup();

	blog(LOG_INFO, "[Time Scheduler] Plugin unloaded");
}
