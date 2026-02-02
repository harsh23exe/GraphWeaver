// rate_limiter.hpp - Per-Host Rate Limiting
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <random>
#include <string>

namespace docscraper::fetch {

class RateLimiter {
public:
    explicit RateLimiter(std::chrono::milliseconds default_delay);

    // Wait until allowed to make request to host
    void wait_for_host(const std::string& host);

    // Set per-host delay override
    void set_host_delay(const std::string& host, std::chrono::milliseconds delay);

    // Get current delay for host
    std::chrono::milliseconds get_host_delay(const std::string& host) const;

    // Reset host state (e.g., when resuming)
    void reset_host(const std::string& host);

private:
    std::chrono::milliseconds default_delay_;
    mutable std::mutex mutex_;
    std::map<std::string, std::chrono::milliseconds> host_delays_;
    std::map<std::string, std::chrono::steady_clock::time_point> last_request_;

    std::chrono::milliseconds add_jitter(std::chrono::milliseconds delay);
};

} // namespace docscraper::fetch
