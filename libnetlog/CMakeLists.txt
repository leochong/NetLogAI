cmake_minimum_required(VERSION 3.16)
project(libnetlog
    VERSION 1.0.0
    DESCRIPTION "Open Source Network Log Parsing Library"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(MSVC)
    # Only add warnings as errors if this is the main project
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        add_compile_options(/W4 /WX)
    else()
        add_compile_options(/W3)  # Lower warning level when used as subdirectory
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build options
option(LIBNETLOG_BUILD_TESTS "Build unit tests" OFF)
option(LIBNETLOG_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(LIBNETLOG_BUILD_EXAMPLES "Build usage examples" OFF)
option(LIBNETLOG_BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Set library type
if(LIBNETLOG_BUILD_SHARED_LIBS)
    set(LIBNETLOG_LIBRARY_TYPE SHARED)
else()
    set(LIBNETLOG_LIBRARY_TYPE STATIC)
endif()

# Find dependencies
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
endif()

# Find Lua for scripting engine
find_package(Lua QUIET)
if(NOT LUA_FOUND)
    # Try unofficial-lua from vcpkg
    find_package(unofficial-lua CONFIG QUIET)
    if(unofficial-lua_FOUND)
        set(LUA_LIBRARIES lua)
        message(STATUS "Found unofficial-lua from vcpkg")
    else()
        message(WARNING "Lua not found - Lua scripting features will be disabled")
        set(LIBNETLOG_ENABLE_LUA OFF)
    endif()
else()
    set(LIBNETLOG_ENABLE_LUA ON)
    message(STATUS "Found Lua: ${LUA_LIBRARIES}")
endif()

# Include directories
include_directories(include)

# Core library source files
set(LIBNETLOG_SOURCES
    src/log_entry.cpp
    src/parser_factory.cpp
    src/device_types.cpp
    src/severity.cpp
    src/parsers/base_parser.cpp
    src/parsers/cisco_ios_parser.cpp
    src/parsers/cisco_nxos_parser.cpp
    src/parsers/cisco_asa_parser.cpp
    src/parsers/generic_syslog_parser.cpp
    src/utils/timestamp_parser.cpp
    src/utils/regex_patterns.cpp
)

set(LIBNETLOG_HEADERS
    include/libnetlog/log_entry.hpp
    include/libnetlog/parser_factory.hpp
    include/libnetlog/device_types.hpp
    include/libnetlog/severity.hpp
    include/libnetlog/parsers/base_parser.hpp
    include/libnetlog/parsers/cisco_ios_parser.hpp
    include/libnetlog/parsers/cisco_nxos_parser.hpp
    include/libnetlog/parsers/cisco_asa_parser.hpp
    include/libnetlog/parsers/generic_syslog_parser.hpp
    include/libnetlog/utils/timestamp_parser.hpp
    include/libnetlog/utils/regex_patterns.hpp
)

# Add Lua sources and headers if enabled
if(LIBNETLOG_ENABLE_LUA)
    list(APPEND LIBNETLOG_SOURCES
        src/lua_engine.cpp
        src/parsers/lua_parser.cpp
    )
    list(APPEND LIBNETLOG_HEADERS
        include/libnetlog/lua_engine.hpp
        include/libnetlog/parsers/lua_parser.hpp
    )
endif()

# Create the main library
add_library(libnetlog ${LIBNETLOG_LIBRARY_TYPE}
    ${LIBNETLOG_SOURCES}
    ${LIBNETLOG_HEADERS}
)

# Set library properties
set_target_properties(libnetlog PROPERTIES
    OUTPUT_NAME "netlog"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Link libraries
target_link_libraries(libnetlog
    PUBLIC
    nlohmann_json::nlohmann_json
)

# Link Lua libraries if available
if(LIBNETLOG_ENABLE_LUA)
    if(unofficial-lua_FOUND)
        target_link_libraries(libnetlog PRIVATE lua)
    elseif(LUA_FOUND)
        target_link_libraries(libnetlog PRIVATE ${LUA_LIBRARIES})
        target_include_directories(libnetlog PRIVATE ${LUA_INCLUDE_DIR})
    endif()
endif()

# Include directories for consumers
target_include_directories(libnetlog
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compile definitions
target_compile_definitions(libnetlog
    PRIVATE
    LIBNETLOG_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    LIBNETLOG_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    LIBNETLOG_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Add Lua support definition
if(LIBNETLOG_ENABLE_LUA)
    target_compile_definitions(libnetlog PRIVATE LIBNETLOG_ENABLE_LUA=1)
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(libnetlog PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Debug configuration
target_compile_definitions(libnetlog PRIVATE
    $<$<CONFIG:Debug>:LIBNETLOG_DEBUG=1>
    $<$<CONFIG:Debug>:LIBNETLOG_ENABLE_LOGGING=1>
)

# Build examples
# if(LIBNETLOG_BUILD_EXAMPLES)
#     add_subdirectory(examples)
# endif()

# Build tests
# if(LIBNETLOG_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# Build benchmarks
if(LIBNETLOG_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Install rules
include(GNUInstallDirs)

# Temporarily disable exports
# install(TARGETS libnetlog
#     EXPORT libnetlogTargets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

install(FILES README.md LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Export targets - disabled for now
# install(EXPORT libnetlogTargets
#     FILE libnetlogTargets.cmake
#     NAMESPACE libnetlog::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libnetlog
# )

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libnetlogConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Temporarily disable config file generation
# configure_package_config_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libnetlogConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/libnetlogConfig.cmake"
#     INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libnetlog
# )
# 
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/libnetlogConfig.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/libnetlogConfigVersion.cmake"
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libnetlog
# )
# 
# # pkg-config support
# configure_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/libnetlog.pc.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/libnetlog.pc"
#     @ONLY
# )
# 
# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libnetlog.pc"
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
# )

# CPack configuration
set(CPACK_PACKAGE_NAME "libnetlog")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "NetLogAI Contributors")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "NetLogAI Contributors")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17)")
endif()

# Disable CPack when used as subdirectory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CPack)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "libnetlog ${PROJECT_VERSION} Configuration Summary:")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library Type: ${LIBNETLOG_LIBRARY_TYPE}")
message(STATUS "  Build Tests: ${LIBNETLOG_BUILD_TESTS}")
message(STATUS "  Build Benchmarks: ${LIBNETLOG_BUILD_BENCHMARKS}")
message(STATUS "  Build Examples: ${LIBNETLOG_BUILD_EXAMPLES}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")