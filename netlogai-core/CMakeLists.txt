cmake_minimum_required(VERSION 3.20)
project(NetLogAI-Core
    VERSION 1.0.0
    DESCRIPTION "NetLogAI Core - Enterprise Network Log Analysis Platform"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)  # High warning level, disable warnings as errors for now
    add_compile_options(/permissive-)  # Disable non-conforming code
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Commercial" CACHE STRING "Available build configurations" FORCE)

# Custom configuration for commercial builds with license validation
if(CMAKE_BUILD_TYPE STREQUAL "Commercial")
    add_compile_definitions(COMMERCIAL_LICENSE_BUILD=1)
    add_compile_definitions(ENABLE_LICENSE_VALIDATION=1)
endif()

# vcpkg integration
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Add vcpkg installed path for manual fallback
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows")

# Find required packages
find_package(unofficial-lua CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Enable Lua scripting when unofficial-lua is found
if(unofficial-lua_FOUND)
    add_compile_definitions(LIBNETLOG_ENABLE_LUA=1)
    message(STATUS "Lua scripting enabled for netlogai-core")
endif()

# Optional packages for AI integration
find_package(CURL QUIET)
find_package(OpenSSL QUIET)

if(CURL_FOUND AND OPENSSL_FOUND)
    message(STATUS "AI integration dependencies found - enabling AI features")
    add_compile_definitions(ENABLE_AI_INTEGRATION=1)
else()
    message(WARNING "AI integration dependencies missing - AI features will be disabled")
endif()

# Include directories
include_directories(include)
include_directories(third-party/libnetlog/include)
include_directories(third-party/plugin-sdk/include)

# Source files organization
file(GLOB_RECURSE CORE_SOURCES
    "src/core/shell/*.cpp"
    "src/core/shell/*.hpp"
    "src/core/ai/*.cpp"
    "src/core/ai/*.hpp"
    "src/core/analysis/*.cpp"
    "src/core/analysis/*.hpp"
    "src/core/plugins/*.cpp"
    "src/core/plugins/*.hpp"
)

file(GLOB_RECURSE COMMON_SOURCES
    "src/common/*.cpp"
    "src/common/*.hpp"
)

# CLI source files
file(GLOB CLI_SOURCES
    "src/cli/*.cpp"
    "src/cli/*.hpp"
)

file(GLOB COMMANDS_SOURCES
    "src/commands/*.cpp"
    "src/commands/*.hpp"
)

# Main executable
add_executable(netlogai
    src/main.cpp
    ${CLI_SOURCES}
    ${COMMANDS_SOURCES}
    ${CORE_SOURCES}
    ${COMMON_SOURCES}
)

# Link libraries
target_link_libraries(netlogai
    PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    fmt::fmt
)

# Link Lua if available
if(unofficial-lua_FOUND)
    target_link_libraries(netlogai PRIVATE lua)
endif()

# Link AI integration libraries if available
if(CURL_FOUND AND OPENSSL_FOUND)
    target_link_libraries(netlogai
        PRIVATE
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

# Link with libnetlog (open source component)
if(EXISTS "${CMAKE_SOURCE_DIR}/third-party/libnetlog/CMakeLists.txt")
    add_subdirectory(third-party/libnetlog)
    target_link_libraries(netlogai PRIVATE libnetlog)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../libnetlog/CMakeLists.txt")
    # Use local libnetlog directory
    add_subdirectory("${CMAKE_SOURCE_DIR}/../libnetlog" "${CMAKE_BINARY_DIR}/libnetlog")
    target_link_libraries(netlogai PRIVATE libnetlog)
    message(STATUS "Using local libnetlog from ../libnetlog")
else()
    message(WARNING "libnetlog not found - neither submodule nor local directory")
endif()

# Plugin SDK integration - disabled for now
# if(EXISTS "${CMAKE_SOURCE_DIR}/third-party/plugin-sdk/CMakeLists.txt")
#     add_subdirectory(third-party/plugin-sdk)
#     target_link_libraries(netlogai PRIVATE plugin-sdk)
# else()
#     message(STATUS "plugin-sdk not available - plugin system will be disabled")
# endif()

# Windows-specific settings
if(WIN32)
    target_compile_definitions(netlogai PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(netlogai PRIVATE ws2_32 advapi32)
endif()

# Debug configuration
target_compile_definitions(netlogai PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD=1>
    $<$<CONFIG:Debug>:ENABLE_VERBOSE_LOGGING=1>
)

# Release configuration
target_compile_definitions(netlogai PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Release>:OPTIMIZE_FOR_PERFORMANCE=1>
)

# Commercial license validation
if(CMAKE_BUILD_TYPE STREQUAL "Commercial")
    # Add license validation source files
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/license/")
        file(GLOB LICENSE_SOURCES "src/license/*.cpp")
        target_sources(netlogai PRIVATE ${LICENSE_SOURCES})
        message(STATUS "Added commercial license validation")
    endif()
endif()

# Install rules
install(TARGETS netlogai
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install configuration files
install(DIRECTORY config/
    DESTINATION etc/netlogai
    FILES_MATCHING PATTERN "*.json.example"
)

# Install documentation
install(FILES README.md LICENSE-COMMERCIAL.txt
    DESTINATION share/doc/netlogai
)

# Testing - disabled for now
# enable_testing()

# Unit tests
# if(EXISTS "${CMAKE_SOURCE_DIR}/src/tests/CMakeLists.txt")
#     add_subdirectory(src/tests)
# endif()

# Custom targets for development
add_custom_target(setup-dev-env
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/setup-dev-env.bat
    COMMENT "Setting up development environment"
)

add_custom_target(update-submodules
    COMMAND git submodule update --recursive --remote
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating git submodules"
)

# Package configuration
set(CPACK_PACKAGE_NAME "NetLogAI-Core")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "NetLogAI Corporation")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE-COMMERCIAL.txt")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "NetLogAI Core ${PROJECT_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://netlogai.com/support")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://netlogai.com")
    set(CPACK_NSIS_CONTACT "support@netlogai.com")
endif()

include(CPack)