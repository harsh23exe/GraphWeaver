// orchestrator.hpp - Parallel site orchestration
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "crawler/config.hpp"
#include <chrono>
#include <string>
#include <vector>

namespace docscraper::orchestrate {

struct SiteResult {
    std::string site_key;
    bool success = true;
    std::string error;
    int64_t pages_processed = 0;
    std::chrono::milliseconds duration{0};
};

class Orchestrator {
public:
    Orchestrator(
        const crawler::AppConfig& app_config,
        const std::vector<std::string>& site_keys,
        bool resume
    );

    std::vector<SiteResult> run();
    void shutdown();

private:
    crawler::AppConfig app_config_;
    std::vector<std::string> site_keys_;
    bool resume_ = false;
};

} // namespace docscraper::orchestrate
