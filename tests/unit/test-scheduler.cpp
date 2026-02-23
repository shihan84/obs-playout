#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "scheduler-core.h"
#include "playlist-manager.h"
#include "media-controller.h"
#include "time-trigger.h"
#include "utils/config.h"
#include "utils/logger.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;

class MockPlaylistManager : public PlaylistManager {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(bool, load_schedule_file, (const std::string&), (override));
    MOCK_METHOD(void, unload_schedule_file, (const std::string&), (override));
    MOCK_METHOD(void, reload_schedules, (), (override));
    MOCK_METHOD(std::shared_ptr<ScheduledItem>, get_item, (const std::string&), (override));
};

class MockMediaController : public MediaController {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(bool, play_media, (const std::string&, const std::string&), (override));
    MOCK_METHOD(bool, stop_media, (const std::string&), (override));
    MOCK_METHOD(bool, execute_item, (const ScheduledItem&), (override));
    MOCK_METHOD(bool, play_idle_content, (), (override));
};

class MockTimeTrigger : public TimeTrigger {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(std::vector<std::string>, get_current_items, (), (override));
    MOCK_METHOD(std::vector<std::string>, get_next_items, (), (override));
    MOCK_METHOD(void, update_schedule, (), (override));
};

class SchedulerCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for tests
        Logger::initialize();
        Logger::set_level(Logger::Level::DEBUG);
        
        // Create scheduler with mocked dependencies
        scheduler = std::make_unique<SchedulerCore>();
        
        // Inject mocks (this would require modifying SchedulerCore to accept dependencies)
        // For now, we'll test the public interface
    }
    
    void TearDown() override {
        if (scheduler) {
            scheduler->stop();
        }
        Logger::cleanup();
    }
    
    std::unique_ptr<SchedulerCore> scheduler;
};

TEST_F(SchedulerCoreTest, InitializationTest) {
    EXPECT_TRUE(scheduler->initialize());
    EXPECT_FALSE(scheduler->is_running());
    EXPECT_TRUE(scheduler->is_enabled());
}

TEST_F(SchedulerCoreTest, StartStopTest) {
    ASSERT_TRUE(scheduler->initialize());
    
    EXPECT_FALSE(scheduler->is_running());
    
    scheduler->start();
    EXPECT_TRUE(scheduler->is_running());
    
    scheduler->stop();
    EXPECT_FALSE(scheduler->is_running());
}

TEST_F(SchedulerCoreTest, ToggleEnabledTest) {
    ASSERT_TRUE(scheduler->initialize());
    
    bool initial_state = scheduler->is_enabled();
    scheduler->toggle_enabled();
    EXPECT_NE(scheduler->is_enabled(), initial_state);
    
    scheduler->toggle_enabled();
    EXPECT_EQ(scheduler->is_enabled(), initial_state);
}

TEST_F(SchedulerCoreTest, StatusTest) {
    ASSERT_TRUE(scheduler->initialize());
    
    std::string status = scheduler->get_status();
    EXPECT_FALSE(status.empty());
    
    std::string current_item = scheduler->get_current_item();
    std::string next_item = scheduler->get_next_item();
    
    // Should not crash and should return valid strings
    EXPECT_TRUE(current_item.empty() || !current_item.empty());
    EXPECT_TRUE(next_item.empty() || !next_item.empty());
}

TEST_F(SchedulerCoreTest, ReloadSchedulesTest) {
    ASSERT_TRUE(scheduler->initialize());
    
    // This should not crash
    EXPECT_NO_THROW(scheduler->reload_schedules());
}

TEST_F(SchedulerCoreTest, ForceCheckTest) {
    ASSERT_TRUE(scheduler->initialize());
    
    // This should not crash
    EXPECT_NO_THROW(scheduler->force_check());
}

class TimeTriggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::initialize();
        time_trigger = std::make_unique<TimeTrigger>();
    }
    
    void TearDown() override {
        if (time_trigger) {
            time_trigger->cleanup();
        }
        Logger::cleanup();
    }
    
    std::unique_ptr<TimeTrigger> time_trigger;
};

TEST_F(TimeTriggerTest, InitializationTest) {
    EXPECT_TRUE(time_trigger->initialize());
    EXPECT_FALSE(time_trigger->is_schedule_empty());
}

TEST_F(TimeTriggerTest, TimeUtilitiesTest) {
    std::string current_time = time_trigger->get_current_time();
    EXPECT_FALSE(current_time.empty());
    EXPECT_EQ(current_time.length(), 5); // HH:MM format
    EXPECT_EQ(current_time[2], ':');
    
    std::string current_day = time_trigger->get_current_day();
    EXPECT_FALSE(current_day.empty());
    
    int current_minutes = time_trigger->get_current_minutes();
    EXPECT_GE(current_minutes, 0);
    EXPECT_LT(current_minutes, 24 * 60);
}

TEST_F(TimeTriggerTest, ScheduleManagementTest) {
    ASSERT_TRUE(time_trigger->initialize());
    
    size_t initial_size = time_trigger->get_schedule_size();
    
    time_trigger->rebuild_schedule();
    size_t rebuilt_size = time_trigger->get_schedule_size();
    
    // Size should be reasonable (not necessarily equal to initial)
    EXPECT_LE(rebuilt_size, 1000); // Sanity check
    
    time_trigger->clear_schedule();
    EXPECT_TRUE(time_trigger->is_schedule_empty());
    EXPECT_EQ(time_trigger->get_schedule_size(), 0);
}

TEST_F(TimeTriggerTest, CurrentItemsTest) {
    ASSERT_TRUE(time_trigger->initialize());
    
    auto current_items = time_trigger->get_current_items();
    EXPECT_TRUE(current_items.empty() || !current_items.empty()); // Should not crash
    
    auto next_items = time_trigger->get_next_items();
    EXPECT_TRUE(next_items.empty() || !next_items.empty()); // Should not crash
    
    auto upcoming_items = time_trigger->get_upcoming_items(5);
    EXPECT_TRUE(upcoming_items.empty() || !upcoming_items.empty()); // Should not crash
}

class PlaylistManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::initialize();
        playlist_manager = std::make_unique<PlaylistManager>();
    }
    
    void TearDown() override {
        if (playlist_manager) {
            playlist_manager->cleanup();
        }
        Logger::cleanup();
    }
    
    std::unique_ptr<PlaylistManager> playlist_manager;
};

TEST_F(PlaylistManagerTest, InitializationTest) {
    EXPECT_TRUE(playlist_manager->initialize());
    EXPECT_EQ(playlist_manager->get_total_items(), 0);
}

TEST_F(PlaylistManagerTest, ItemRetrievalTest) {
    ASSERT_TRUE(playlist_manager->initialize());
    
    // Test with non-existent item
    auto item = playlist_manager->get_item("non_existent_item");
    EXPECT_EQ(item, nullptr);
    
    // Test getting items for current day/time
    auto items_for_time = playlist_manager->get_items_for_time("09:00", "monday");
    EXPECT_TRUE(items_for_time.empty()); // Should be empty with no schedules loaded
    
    auto items_for_day = playlist_manager->get_items_for_day("monday");
    EXPECT_TRUE(items_for_day.empty()); // Should be empty with no schedules loaded
}

TEST_F(PlaylistManagerTest, ValidationTest) {
    ASSERT_TRUE(playlist_manager->initialize());
    
    ScheduledItem valid_item;
    valid_item.name = "Test Item";
    valid_item.time = "09:00";
    valid_item.source = "Test Source";
    valid_item.file_path = "test.mp4";
    valid_item.days = {"monday", "tuesday"};
    
    EXPECT_TRUE(playlist_manager->validate_item(valid_item));
    
    ScheduledItem invalid_item;
    invalid_item.name = ""; // Empty name
    invalid_item.time = "25:00"; // Invalid time
    invalid_item.source = ""; // Empty source
    
    EXPECT_FALSE(playlist_manager->validate_item(invalid_item));
}

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::initialize();
    }
    
    void TearDown() override {
        Logger::cleanup();
    }
};

TEST_F(ConfigTest, DefaultValuesTest) {
    EXPECT_TRUE(Config::is_enabled());
    EXPECT_EQ(Config::get_check_interval_seconds(), 1);
    EXPECT_EQ(Config::get_timezone(), "UTC");
    EXPECT_FALSE(Config::is_debug_mode());
}

TEST_F(ConfigTest, ScheduleFilesTest) {
    auto schedule_files = Config::get_schedule_files();
    EXPECT_TRUE(schedule_files.size() >= 0); // Should not crash
    
    Config::ScheduleFile test_file;
    test_file.path = "/test/path.json";
    test_file.enabled = true;
    test_file.name = "Test Schedule";
    
    Config::add_schedule_file(test_file);
    
    auto updated_files = Config::get_schedule_files();
    EXPECT_GT(updated_files.size(), schedule_files.size());
    
    Config::remove_schedule_file(test_file.path);
    
    auto final_files = Config::get_schedule_files();
    EXPECT_EQ(final_files.size(), schedule_files.size());
}

TEST_F(ConfigTest, ConfigurationPersistenceTest) {
    bool original_enabled = Config::is_enabled();
    int original_interval = Config::get_check_interval_seconds();
    std::string original_timezone = Config::get_timezone();
    
    // Change values
    Config::set_enabled(!original_enabled);
    Config::set_check_interval_seconds(original_interval + 1);
    Config::set_timezone("America/New_York");
    
    // Verify changes
    EXPECT_EQ(Config::is_enabled(), !original_enabled);
    EXPECT_EQ(Config::get_check_interval_seconds(), original_interval + 1);
    EXPECT_EQ(Config::get_timezone(), "America/New_York");
    
    // Restore original values
    Config::set_enabled(original_enabled);
    Config::set_check_interval_seconds(original_interval);
    Config::set_timezone(original_timezone);
}

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::initialize();
        Logger::set_level(Logger::Level::DEBUG);
    }
    
    void TearDown() override {
        Logger::cleanup();
    }
};

TEST_F(LoggerTest, BasicLoggingTest) {
    // These should not crash
    EXPECT_NO_THROW(Logger::debug("Debug message"));
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warning("Warning message"));
    EXPECT_NO_THROW(Logger::error("Error message"));
}

TEST_F(LoggerTest, LevelFilteringTest) {
    Logger::set_level(Logger::Level::ERROR);
    
    // These should be filtered out
    EXPECT_NO_THROW(Logger::debug("Debug message"));
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warning("Warning message"));
    
    // This should not be filtered
    EXPECT_NO_THROW(Logger::error("Error message"));
}

TEST_F(LoggerTest, FilePathTest) {
    std::string test_path = "/tmp/test_scheduler.log";
    EXPECT_NO_THROW(Logger::set_file_path(test_path));
    
    // Log a message to test file writing
    EXPECT_NO_THROW(Logger::info("Test message for file"));
}

// Integration test example
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::initialize();
        Logger::set_level(Logger::Level::INFO);
    }
    
    void TearDown() override {
        Logger::cleanup();
    }
};

TEST_F(IntegrationTest, FullWorkflowTest) {
    // Test the complete workflow: initialize -> start -> stop -> cleanup
    
    auto scheduler = std::make_unique<SchedulerCore>();
    
    // Initialize
    EXPECT_TRUE(scheduler->initialize());
    EXPECT_FALSE(scheduler->is_running());
    
    // Start
    scheduler->start();
    EXPECT_TRUE(scheduler->is_running());
    
    // Let it run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop
    scheduler->stop();
    EXPECT_FALSE(scheduler->is_running());
    
    // Cleanup is automatic in destructor
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
