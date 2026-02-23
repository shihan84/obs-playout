# Tests CMakeLists.txt for obs-time-scheduler

cmake_minimum_required(VERSION 3.20)

# Find required packages for testing
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # Use FetchContent if not found system-wide
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Enable testing
enable_testing()

# Unit tests
add_executable(unit_tests
    unit/test-scheduler.cpp
    unit/test-playlist.cpp
    unit/test-time-trigger.cpp
    unit/test-media-controller.cpp
    unit/test-config.cpp
    unit/test-logger.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${libobs_INCLUDE_DIRS}
    ${obs-frontend-api_INCLUDE_DIRS}
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# Add mock OBS functions for testing
target_sources(unit_tests PRIVATE
    unit/mocks/obs-mock.cpp
)

# Integration tests
add_executable(integration_tests
    integration/test-obs-control.cpp
    integration/test-schedule-execution.cpp
    integration/test-ui-interaction.cpp
)

target_include_directories(integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${libobs_INCLUDE_DIRS}
    ${obs-frontend-api_INCLUDE_DIRS}
)

target_link_libraries(integration_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    libobs
    obs-frontend-api
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# Add tests to CTest
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)

# Set test properties
set_tests_properties(UnitTests PROPERTIES
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 600
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "OBS_HEADLESS=1"
)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS unit_tests integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
