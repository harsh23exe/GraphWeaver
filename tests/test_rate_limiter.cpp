// test_rate_limiter.cpp - Unit tests for rate limiter
#include <gtest/gtest.h>
#include "fetch/rate_limiter.hpp"

using namespace docscraper::fetch;

TEST(RateLimiterTest, BasicDelay) {
    RateLimiter limiter(std::chrono::milliseconds(100));

    auto start = std::chrono::steady_clock::now();
    limiter.wait_for_host("example.com");
    limiter.wait_for_host("example.com");  // Should wait ~100ms
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_GE(elapsed, std::chrono::milliseconds(90));  // Allow jitter
}

TEST(RateLimiterTest, PerHostDelay) {
    RateLimiter limiter(std::chrono::milliseconds(100));
    limiter.set_host_delay("example.com", std::chrono::milliseconds(200));

    auto start = std::chrono::steady_clock::now();
    limiter.wait_for_host("example.com");
    limiter.wait_for_host("example.com");
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_GE(elapsed, std::chrono::milliseconds(180));  // Allow jitter
}

TEST(RateLimiterTest, DifferentHostsIndependent) {
    RateLimiter limiter(std::chrono::milliseconds(100));

    auto start = std::chrono::steady_clock::now();
    limiter.wait_for_host("example.com");
    limiter.wait_for_host("other.com");  // Different host, no wait
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, std::chrono::milliseconds(80));
}
