# CMakeLists.txt - Doc Scraper C++ Build Configuration
cmake_minimum_required(VERSION 3.20)

# Set policy for compatibility with older cmake_minimum_required in dependencies
cmake_policy(SET CMP0057 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for dependencies")

project(doc-scraper VERSION 1.0.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_WITH_SANITIZERS "Build with address and undefined behavior sanitizers" OFF)
option(ENABLE_LLM_GUIDANCE "Enable LLM-guided crawling feature" ON)

# Sanitizer support
if(BUILD_WITH_SANITIZERS)
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# Required: Threads
find_package(Threads REQUIRED)

# FetchContent for external dependencies
include(FetchContent)

# spdlog for logging
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# yaml-cpp for config parsing
FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

# nlohmann/json for JSON handling
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# cpp-httplib for HTTP client (header-only)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.0
)
FetchContent_MakeAvailable(httplib)

# cxxopts for CLI argument parsing
FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(cxxopts)

# gumbo-parser for HTML parsing (Google's official repo)
FetchContent_Declare(gumbo
    GIT_REPOSITORY https://github.com/nicholaszhan/gumbo-parser.git
    GIT_TAG v0.10.1
)
# Gumbo needs special handling - we'll use system library or build manually
# For now, try to find system gumbo or use libxml2 as alternative
find_library(GUMBO_LIB gumbo)
find_path(GUMBO_INCLUDE_DIR gumbo.h)
if(GUMBO_LIB AND GUMBO_INCLUDE_DIR)
    add_library(gumbo INTERFACE)
    target_include_directories(gumbo INTERFACE ${GUMBO_INCLUDE_DIR})
    target_link_libraries(gumbo INTERFACE ${GUMBO_LIB})
    message(STATUS "Found system gumbo: ${GUMBO_LIB}")
else()
    message(STATUS "Gumbo not found. Will use libxml2 for HTML parsing.")
    find_package(LibXml2)
    if(LibXml2_FOUND)
        add_library(gumbo ALIAS LibXml2::LibXml2)
    else()
        message(STATUS "Neither gumbo nor libxml2 found. HTML parsing will be limited.")
        add_library(gumbo INTERFACE)
    endif()
endif()

# RocksDB - try to find system installation first
find_package(RocksDB QUIET)
if(NOT RocksDB_FOUND)
    # Fall back to pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ROCKSDB rocksdb)
    endif()
    
    if(NOT ROCKSDB_FOUND)
        message(STATUS "RocksDB not found. Will attempt to build from source or you need to install it.")
        message(STATUS "  macOS: brew install rocksdb")
        message(STATUS "  Linux: sudo apt-get install librocksdb-dev")
        
        # For now, create a placeholder target
        # In production, you'd want FetchContent for RocksDB too
        add_library(rocksdb INTERFACE)
        target_include_directories(rocksdb INTERFACE /usr/local/include /opt/homebrew/include)
        target_link_directories(rocksdb INTERFACE /usr/local/lib /opt/homebrew/lib)
        target_link_libraries(rocksdb INTERFACE -lrocksdb)
    else()
        add_library(rocksdb INTERFACE)
        target_include_directories(rocksdb INTERFACE ${ROCKSDB_INCLUDE_DIRS})
        target_link_directories(rocksdb INTERFACE ${ROCKSDB_LIBRARY_DIRS})
        target_link_libraries(rocksdb INTERFACE ${ROCKSDB_LIBRARIES})
    endif()
else()
    add_library(rocksdb ALIAS RocksDB::rocksdb)
endif()

# OpenSSL for hashing
find_package(OpenSSL REQUIRED)

# pugixml for XML/sitemap parsing
FetchContent_Declare(pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
)
FetchContent_MakeAvailable(pugixml)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files - collect all .cpp files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# Exclude main.cpp for library
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Create the main library
add_library(doc_scraper_lib STATIC ${SOURCES})
target_link_libraries(doc_scraper_lib PUBLIC
    Threads::Threads
    spdlog::spdlog
    yaml-cpp
    nlohmann_json::nlohmann_json
    httplib::httplib
    gumbo
    rocksdb
    OpenSSL::Crypto
    pugixml
)

# Main executable
add_executable(crawler src/main.cpp)
target_link_libraries(crawler PRIVATE
    doc_scraper_lib
    cxxopts
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # Google Test
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Add test subdirectory
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS crawler DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Print configuration summary
message(STATUS "")
message(STATUS "=== doc-scraper Configuration ===")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Sanitizers: ${BUILD_WITH_SANITIZERS}")
message(STATUS "  LLM Guidance: ${ENABLE_LLM_GUIDANCE}")
message(STATUS "=================================")
message(STATUS "")
