// crawler.hpp - Core Crawler Engine
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "crawler/config.hpp"
#include "storage/store_interface.hpp"
#include "fetch/fetcher.hpp"
#include "fetch/rate_limiter.hpp"
#include "queue/priority_queue.hpp"
#include "process/content_processor.hpp"
#include <atomic>
#include <thread>

namespace docscraper::crawler {

class Crawler {
public:
    Crawler(
        const AppConfig& app_config,
        const SiteConfig& site_config,
        const std::string& site_key,
        storage::VisitedStore& store,
        fetch::Fetcher& fetcher,
        fetch::RateLimiter& rate_limiter,
        bool resume
    );

    ~Crawler();

    void run();
    void shutdown();

    int64_t get_pages_processed() const { return pages_processed_.load(); }

private:
    AppConfig app_config_;
    SiteConfig site_config_;
    std::string site_key_;
    std::string site_output_dir_;

    storage::VisitedStore& store_;
    fetch::Fetcher& fetcher_;
    fetch::RateLimiter& rate_limiter_;

    queue::ThreadSafePriorityQueue queue_;
    process::ImageProcessor image_processor_;
    process::ContentProcessor content_processor_;

    std::vector<std::thread> workers_;
    std::atomic<bool> shutdown_flag_{false};
    std::atomic<int64_t> pages_processed_{0};
    std::atomic<int64_t> in_flight_{0};

    void seed_queue();
    void worker_loop();
    void process_page(const models::WorkItem& item);

    bool should_stop() const { return shutdown_flag_.load(); }
};

} // namespace docscraper::crawler
