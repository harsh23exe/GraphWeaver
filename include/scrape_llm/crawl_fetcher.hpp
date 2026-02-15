#pragma once

#include "scrape_llm/cli_config.hpp"
#include "scrape_llm/ssrf_guard.hpp"
#include "fetch/rate_limiter.hpp"
#include "fetch/robots.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>
#include <optional>

namespace scrapellm {

struct CrawlResult {
    std::string url;
    std::string normalized_url;
    int depth = 0;
    std::string html;
    std::string final_url;
    bool success = false;
    std::string error;
};

// Fetches pages with rate limiting, robots, SSRF guard, and optional disk cache.
class CrawlFetcher {
public:
    explicit CrawlFetcher(const RunConfig& config);

    // Fetch robots.txt for the host of base_url; parses and stores. Returns body (or empty if failed).
    std::string fetch_robots(const std::string& base_url);

    // Fetch one URL. Respects rate limit and robots. Returns nullopt if blocked or error.
    std::optional<CrawlResult> fetch(const std::string& url, int depth);

    // Check whether URL is allowed by robots (call after fetch_robots for that host).
    bool is_allowed_by_robots(const std::string& url) const;

    // Normalize and dedupe: returns true if url was new and should be crawled.
    bool seen_add(const std::string& normalized_url);

    const RunConfig& config() const { return config_; }

private:
    RunConfig config_;
    docscraper::fetch::RateLimiter rate_limiter_;
    docscraper::fetch::RobotsHandler robots_;
    std::string robots_origin_;  // scheme+authority for which robots was loaded
    std::unordered_set<std::string> seen_urls_;
    mutable std::string cache_dir_;
    std::string user_agent_;

    std::string cache_path_for(const std::string& normalized_url) const;
    bool load_from_cache(const std::string& normalized_url, std::string& out_html) const;
    void save_to_cache(const std::string& normalized_url, const std::string& html);
};

} // namespace scrapellm
