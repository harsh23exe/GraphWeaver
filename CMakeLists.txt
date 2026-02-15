# CMakeLists.txt - GraphWeaver (scrape-llm)
cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0057 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for dependencies")

project(GraphWeaver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_WITH_SANITIZERS "Build with address and undefined behavior sanitizers" OFF)

if(BUILD_WITH_SANITIZERS)
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

include(FetchContent)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.0
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(cxxopts)

FetchContent_Declare(gumbo
    GIT_REPOSITORY https://github.com/nicholaszhan/gumbo-parser.git
    GIT_TAG v0.10.1
)
find_library(GUMBO_LIB gumbo)
find_path(GUMBO_INCLUDE_DIR gumbo.h)
if(GUMBO_LIB AND GUMBO_INCLUDE_DIR)
    add_library(gumbo INTERFACE)
    target_include_directories(gumbo INTERFACE ${GUMBO_INCLUDE_DIR})
    target_link_libraries(gumbo INTERFACE ${GUMBO_LIB})
    message(STATUS "Found system gumbo: ${GUMBO_LIB}")
else()
    find_package(LibXml2)
    if(LibXml2_FOUND)
        add_library(gumbo ALIAS LibXml2::LibXml2)
    else()
        add_library(gumbo INTERFACE)
        message(STATUS "Neither gumbo nor libxml2 found. HTML parsing will be limited.")
    endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# Core library: parse, utils, fetch (only what scrape-llm needs)
set(DOC_SCRAPER_SOURCES
    src/parse/html_parser.cpp
    src/parse/normalizer.cpp
    src/utils/hash.cpp
    src/fetch/rate_limiter.cpp
    src/fetch/robots.cpp
)
add_library(doc_scraper_lib STATIC ${DOC_SCRAPER_SOURCES})
target_link_libraries(doc_scraper_lib PUBLIC
    Threads::Threads
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    httplib::httplib
    gumbo
    OpenSSL::Crypto
)

set(SCRAPE_LLM_LIB_SOURCES
    src/scrape_llm/cli_config.cpp
    src/scrape_llm/ssrf_guard.cpp
    src/scrape_llm/crawl_fetcher.cpp
    src/scrape_llm/content_extractor.cpp
    src/scrape_llm/llm_client.cpp
    src/scrape_llm/pipeline.cpp
    src/scrape_llm/schema_infer.cpp
    src/scrape_llm/relevance_router.cpp
    src/scrape_llm/record_parser.cpp
    src/scrape_llm/validator.cpp
    src/scrape_llm/output_writers.cpp
    src/scrape_llm/report_generator.cpp
)
add_library(scrape_llm_lib STATIC ${SCRAPE_LLM_LIB_SOURCES})
target_link_libraries(scrape_llm_lib PUBLIC doc_scraper_lib CURL::libcurl cxxopts)

add_executable(scrape-llm src/scrape_llm/main_scrape_llm.cpp)
target_link_libraries(scrape-llm PRIVATE scrape_llm_lib cxxopts)

if(BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

install(TARGETS scrape-llm DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

message(STATUS "")
message(STATUS "=== GraphWeaver Configuration ===")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Targets: scrape-llm")
message(STATUS "=================================")
message(STATUS "")
