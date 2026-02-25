cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

# Basic compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Basic build settings
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# Helper function for setting plugin properties
function(set_target_properties_plugin target)
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/obs-plugins/64bit/"
  )
endfunction()

# Find required OBS dependencies
find_package(libobs REQUIRED)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
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

# Create plugin library
add_library(${CMAKE_PROJECT_NAME} MODULE)

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

# Add source files
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SOURCES})

# Set plugin properties
set_target_properties_plugin(${CMAKE_PROJECT_NAME})
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${libobs_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    OBS::libobs
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(ENABLE_FRONTEND_API)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        ws2_32
        shlwapi
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        pthread
    )
elseif(APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        "-framework Foundation"
    )
endif()

# Set plugin properties
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${_name}
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/obs-plugins/64bit/"
)

# Install targets
install(TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION "obs-plugins/64bit/"
)

# Install data files
install(DIRECTORY data/
    DESTINATION "data/obs-plugins/obs-time-scheduler/"
)
