cmake_minimum_required(VERSION 3.20)
project(obs-time-scheduler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required OBS dependencies
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)

# Find additional dependencies
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # Use FetchContent if not found system-wide
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(fmt QUIET)
if(NOT fmt_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.2.1
    )
    FetchContent_MakeAvailable(fmt)
endif()

# Source files
set(SOURCES
    src/plugin-main.cpp
    src/scheduler-core.cpp
    src/playlist-manager.cpp
    src/time-trigger.cpp
    src/media-controller.cpp
    src/utils/file-watcher.cpp
    src/utils/logger.cpp
    src/utils/config.cpp
)

# Header files
set(HEADERS
    src/scheduler-core.h
    src/playlist-manager.h
    src/time-trigger.h
    src/media-controller.h
    src/utils/file-watcher.h
    src/utils/logger.h
    src/utils/config.h
)

# UI components (optional)
option(ENABLE_UI "Build UI components" ON)
if(ENABLE_UI)
    list(APPEND SOURCES
        src/ui/settings-dialog.cpp
        src/ui/schedule-editor.cpp
    )
    list(APPEND HEADERS
        src/ui/settings-dialog.h
        src/ui/schedule-editor.h
    )
    
    # Find Qt6 for UI components
    find_package(Qt6 QUIET COMPONENTS Core Widgets)
    if(Qt6_FOUND)
        target_link_libraries(obs-time-scheduler PRIVATE Qt6::Core Qt6::Widgets)
    endif()
endif()

# Create plugin library
add_library(obs-time-scheduler MODULE ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(obs-time-scheduler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${libobs_INCLUDE_DIRS}
    ${obs-frontend-api_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(obs-time-scheduler PRIVATE
    libobs
    obs-frontend-api
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(obs-time-scheduler PRIVATE
        ws2_32
        shlwapi
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(obs-time-scheduler PRIVATE
        pthread
    )
elseif(APPLE)
    target_link_libraries(obs-time-scheduler PRIVATE
        "-framework Foundation"
    )
endif()

# Set plugin properties
set_target_properties(obs-time-scheduler PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-time-scheduler"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/obs-plugins/64bit/"
)

# Install targets
install(TARGETS obs-time-scheduler
    LIBRARY DESTINATION "obs-plugins/64bit/"
)

# Install data files
install(DIRECTORY data/
    DESTINATION "data/obs-plugins/obs-time-scheduler/"
)

# Add tests
enable_testing()
add_subdirectory(tests)
