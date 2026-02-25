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
# Try to find libobs using package configuration first
find_package(libobs QUIET)

# If package configuration not found, find manually
if(NOT libobs_FOUND)
  message(STATUS "libobs package not found, searching manually...")
  
  # Set possible OBS installation paths
  set(OBS_POSSIBLE_PATHS
    "C:/obs-studio"
    "C:/Program Files/obs-studio"
    "C:/Program Files (x86)/obs-studio"
    "$ENV{CMAKE_PREFIX_PATH}"
    "$ENV{OBS_STUDIO_PATH}"
  )
  
  # Debug: Show what paths we're searching
  message(STATUS "Searching OBS in paths: ${OBS_POSSIBLE_PATHS}")
  
  # Find libobs
  find_path(libobs_INCLUDE_DIR
    NAMES obs/obs-module.h
    PATHS ${OBS_POSSIBLE_PATHS}
    PATH_SUFFIXES include include/obs include/libobs
    DOC "libobs include directory"
  )
  
  find_library(libobs_LIBRARY
    NAMES obs libobs
    PATHS ${OBS_POSSIBLE_PATHS}
    PATH_SUFFIXES lib lib/64bit bin/64bit obs-studio/lib obs-studio/lib/64bit
    DOC "libobs library"
  )
  
  # Debug: Show what we found
  message(STATUS "libobs include dir: ${libobs_INCLUDE_DIR}")
  message(STATUS "libobs library: ${libobs_LIBRARY}")
  
  # Create imported target
  if(libobs_INCLUDE_DIR AND libobs_LIBRARY)
    add_library(OBS::libobs UNKNOWN IMPORTED)
    set_target_properties(OBS::libobs PROPERTIES
      IMPORTED_LOCATION "${libobs_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${libobs_INCLUDE_DIR}"
    )
    set(libobs_FOUND TRUE)
    message(STATUS "Found libobs: ${libobs_LIBRARY}")
  else()
    # Try to find OBS in common locations with different structure
    message(STATUS "Standard search failed, trying alternative paths...")
    
    # Try alternative search for winget installation
    find_path(libobs_INCLUDE_DIR_ALT
      NAMES obs/obs-module.h
      PATHS "C:/Program Files/obs-studio"
      PATH_SUFFIXES "data/obs-plugins/frontend-api" "lib/obs" "include"
      NO_DEFAULT_PATH
    )
    
    find_library(libobs_LIBRARY_ALT
      NAMES obs libobs
      PATHS "C:/Program Files/obs-studio"
      PATH_SUFFIXES "bin/64bit" "lib" "lib/64bit" "obs-studio/bin/64bit"
      NO_DEFAULT_PATH
    )
    
    message(STATUS "Alternative search - include: ${libobs_INCLUDE_DIR_ALT}")
    message(STATUS "Alternative search - library: ${libobs_LIBRARY_ALT}")
    
    if(libobs_INCLUDE_DIR_ALT AND libobs_LIBRARY_ALT)
      add_library(OBS::libobs UNKNOWN IMPORTED)
      set_target_properties(OBS::libobs PROPERTIES
        IMPORTED_LOCATION "${libobs_LIBRARY_ALT}"
        INTERFACE_INCLUDE_DIRECTORIES "${libobs_INCLUDE_DIR_ALT}"
      )
      set(libobs_FOUND TRUE)
      message(STATUS "Found libobs via alternative search: ${libobs_LIBRARY_ALT}")
    else()
      message(FATAL_ERROR "libobs not found in any location. Please install OBS Studio development files.")
    endif()
  endif()
endif()

if(ENABLE_FRONTEND_API)
  # Try to find obs-frontend-api using package configuration first
  find_package(obs-frontend-api QUIET)
  
  # If package configuration not found, find manually
  if(NOT obs-frontend-api_FOUND)
    message(STATUS "obs-frontend-api package not found, searching manually...")
    
    # Find obs-frontend-api
    find_path(obs-frontend-api_INCLUDE_DIR
      NAMES obs-frontend-api/obs-frontend-api.h
      PATHS ${OBS_POSSIBLE_PATHS}
      PATH_SUFFIXES include
      DOC "obs-frontend-api include directory"
    )
    
    find_library(obs-frontend-api_LIBRARY
      NAMES obs-frontend-api
      PATHS ${OBS_POSSIBLE_PATHS}
      PATH_SUFFIXES lib lib/64bit bin/64bit
      DOC "obs-frontend-api library"
    )
    
    # Create imported target
    if(obs-frontend-api_INCLUDE_DIR AND obs-frontend-api_LIBRARY)
      add_library(OBS::obs-frontend-api UNKNOWN IMPORTED)
      set_target_properties(OBS::obs-frontend-api PROPERTIES
        IMPORTED_LOCATION "${obs-frontend-api_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${obs-frontend-api_INCLUDE_DIR}"
      )
      set(obs-frontend-api_FOUND TRUE)
      message(STATUS "Found obs-frontend-api: ${obs-frontend-api_LIBRARY}")
    else()
      message(WARNING "obs-frontend-api not found. Frontend API features will be disabled.")
      set(ENABLE_FRONTEND_API OFF CACHE BOOL "Enable frontend API" FORCE)
    endif()
  endif()
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
