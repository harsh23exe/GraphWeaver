// fetcher.hpp - HTTP Fetcher with Retry Logic
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "crawler/config.hpp"
#include "models/models.hpp"
#include "utils/errors.hpp"
#include <httplib.h>
#include <memory>
#include <string>

namespace docscraper::fetch {

class Fetcher {
public:
    explicit Fetcher(const crawler::AppConfig& config);

    // Fetch with retry logic
    models::FetchResult fetch_with_retry(const std::string& url);

    // Fetch without retries (single attempt)
    models::FetchResult fetch_once(const std::string& url);

private:
    crawler::AppConfig config_;

    bool is_retryable_status(int status_code) const;
    std::chrono::milliseconds calculate_backoff(int attempt) const;
    std::unique_ptr<httplib::Client> build_client(const std::string& base_url);
    std::string extract_base_url(const std::string& url) const;
};

} // namespace docscraper::fetch
