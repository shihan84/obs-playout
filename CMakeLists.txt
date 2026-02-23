cmake_minimum_required(VERSION 3.20)

project(obs-time-scheduler VERSION 1.0.0)

# Plugin options
option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

# Find required OBS dependencies
find_package(libobs REQUIRED)
target_link_libraries(obs-time-scheduler PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(obs-time-scheduler PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(obs-time-scheduler PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    obs-time-scheduler
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    obs-time-scheduler
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

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
    target_link_libraries(obs-time-scheduler PRIVATE nlohmann_json::nlohmann_json)
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
    target_link_libraries(obs-time-scheduler PRIVATE fmt::fmt)
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

# UI components (conditional)
if(ENABLE_QT)
    list(APPEND SOURCES
        src/ui/settings-dialog.cpp
        src/ui/schedule-editor.cpp
    )
    list(APPEND HEADERS
        src/ui/settings-dialog.h
        src/ui/schedule-editor.h
    )
endif()

# Create plugin library
add_library(obs-time-scheduler MODULE ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(obs-time-scheduler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${libobs_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(obs-time-scheduler PRIVATE
    libobs
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(ENABLE_FRONTEND_API)
    target_link_libraries(obs-time-scheduler PRIVATE obs-frontend-api)
endif()

if(ENABLE_QT)
    target_link_libraries(obs-time-scheduler PRIVATE Qt6::Core Qt6::Widgets)
endif()

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
