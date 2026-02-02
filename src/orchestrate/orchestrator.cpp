// orchestrator.cpp - Parallel site orchestration
// LLM Documentation Scraper - C++ Implementation

#include "orchestrate/orchestrator.hpp"
#include "crawler/crawler.hpp"
#include "fetch/fetcher.hpp"
#include "fetch/rate_limiter.hpp"
#include "storage/rocksdb_store.hpp"
#include <thread>
#include <mutex>

namespace docscraper::orchestrate {

Orchestrator::Orchestrator(
    const crawler::AppConfig& app_config,
    const std::vector<std::string>& site_keys,
    bool resume
)
    : app_config_(app_config)
    , site_keys_(site_keys)
    , resume_(resume) {}

std::vector<SiteResult> Orchestrator::run() {
    std::vector<SiteResult> results;
    std::mutex results_mutex;
    std::vector<std::thread> threads;

    for (const auto& site_key : site_keys_) {
        threads.emplace_back([&, site_key]() {
            SiteResult result;
            result.site_key = site_key;
            auto start = std::chrono::steady_clock::now();

            try {
                const auto& site_config = app_config_.get_site(site_key);
                storage::RocksDBStore store(app_config_.state_dir, site_config.allowed_domain, resume_);
                fetch::Fetcher fetcher(app_config_);
                fetch::RateLimiter limiter(app_config_.default_delay_per_host);

                crawler::Crawler crawler(app_config_, site_config, site_key, store, fetcher, limiter, resume_);
                crawler.run();

                result.pages_processed = crawler.get_pages_processed();
                result.success = true;
            } catch (const std::exception& e) {
                result.success = false;
                result.error = e.what();
            }

            auto end = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::lock_guard<std::mutex> lock(results_mutex);
            results.push_back(result);
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    return results;
}

void Orchestrator::shutdown() {
    // Placeholder: individual crawlers handle their own shutdown
}

} // namespace docscraper::orchestrate
