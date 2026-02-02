// rate_limiter.cpp - Rate Limiter Implementation
// LLM Documentation Scraper - C++ Implementation

#include "fetch/rate_limiter.hpp"
#include <thread>

namespace docscraper::fetch {

RateLimiter::RateLimiter(std::chrono::milliseconds default_delay)
    : default_delay_(default_delay) {}

void RateLimiter::set_host_delay(const std::string& host, std::chrono::milliseconds delay) {
    std::lock_guard<std::mutex> lock(mutex_);
    host_delays_[host] = delay;
}

std::chrono::milliseconds RateLimiter::get_host_delay(const std::string& host) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = host_delays_.find(host);
    if (it != host_delays_.end()) {
        return it->second;
    }
    return default_delay_;
}

void RateLimiter::reset_host(const std::string& host) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_request_.erase(host);
}

std::chrono::milliseconds RateLimiter::add_jitter(std::chrono::milliseconds delay) {
    // Add +/- 10% jitter
    int64_t base = delay.count();
    int64_t jitter = static_cast<int64_t>(base * 0.1);
    if (jitter <= 0) return delay;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dist(-jitter, jitter);
    return std::chrono::milliseconds(base + dist(gen));
}

void RateLimiter::wait_for_host(const std::string& host) {
    std::chrono::milliseconds delay;
    std::chrono::steady_clock::time_point last;
    bool has_last = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto delay_it = host_delays_.find(host);
        delay = (delay_it != host_delays_.end()) ? delay_it->second : default_delay_;
        auto last_it = last_request_.find(host);
        if (last_it != last_request_.end()) {
            last = last_it->second;
            has_last = true;
        }
        last_request_[host] = std::chrono::steady_clock::now();
    }

    if (!has_last) {
        return;  // First request, no wait
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
    auto target_delay = add_jitter(delay);

    if (elapsed < target_delay) {
        std::this_thread::sleep_for(target_delay - elapsed);
    }
}

} // namespace docscraper::fetch
