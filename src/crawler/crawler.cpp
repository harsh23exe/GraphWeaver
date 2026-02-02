// crawler.cpp - Core Crawler Engine
// LLM Documentation Scraper - C++ Implementation

#include "crawler/crawler.hpp"
#include "parse/normalizer.hpp"
#include "utils/hash.hpp"
#include <filesystem>
#include <thread>

namespace docscraper::crawler {

Crawler::Crawler(
    const AppConfig& app_config,
    const SiteConfig& site_config,
    const std::string& site_key,
    storage::VisitedStore& store,
    fetch::Fetcher& fetcher,
    fetch::RateLimiter& rate_limiter,
    bool /*resume*/
)
    : app_config_(app_config)
    , site_config_(site_config)
    , site_key_(site_key)
    , site_output_dir_(app_config.output_base_dir + "/" + site_config.allowed_domain)
    , store_(store)
    , fetcher_(fetcher)
    , rate_limiter_(rate_limiter)
    , image_processor_(app_config, store)
    , content_processor_(image_processor_, app_config) {
    std::filesystem::create_directories(site_output_dir_);
}

Crawler::~Crawler() {
    shutdown();
}

void Crawler::seed_queue() {
    for (const auto& url : site_config_.start_urls) {
        if (!parse::URLNormalizer::is_in_scope(url, site_config_.allowed_domain, site_config_.allowed_path_prefix)) {
            continue;
        }
        queue_.push(models::WorkItem(url, 0));
    }
}

void Crawler::run() {
    seed_queue();

    int worker_count = app_config_.num_workers;
    if (worker_count < 1) worker_count = 1;

    for (int i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this]() { worker_loop(); });
    }

    // Monitor queue until empty and no in-flight work
    while (!should_stop()) {
        if (queue_.empty() && in_flight_.load() == 0) {
            queue_.close();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void Crawler::shutdown() {
    shutdown_flag_.store(true);
    queue_.close();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
}

void Crawler::worker_loop() {
    while (!should_stop()) {
        auto item = queue_.try_pop(std::chrono::milliseconds(200));
        if (!item.has_value()) {
            if (queue_.is_closed()) break;
            continue;
        }
        in_flight_++;
        process_page(*item);
        in_flight_--;
    }
}

void Crawler::process_page(const models::WorkItem& item) {
    if (site_config_.max_depth > 0 && item.depth > site_config_.max_depth) {
        return;
    }

    std::string normalized = parse::URLNormalizer::normalize(item.url);
    if (!store_.mark_page_visited(normalized)) {
        return;  // Already visited
    }

    // Rate limit per host
    auto host = parse::URLNormalizer::extract_domain(item.url);
    if (!host.empty()) {
        rate_limiter_.wait_for_host(host);
    }

    auto fetch_result = fetcher_.fetch_with_retry(item.url);
    if (!fetch_result.success) {
        models::PageDBEntry entry;
        entry.status = models::PageStatus::Failure;
        entry.error_type = models::ErrorType::HTTPError;
        entry.error_message = fetch_result.error;
        entry.normalized_url = normalized;
        store_.update_page_status(normalized, entry);
        return;
    }

    if (!fetch_result.is_html()) {
        models::PageDBEntry entry;
        entry.status = models::PageStatus::Success;
        entry.normalized_url = normalized;
        store_.update_page_status(normalized, entry);
        return;
    }

    parse::HTMLDocument doc(fetch_result.body);
    auto result = content_processor_.extract_process_and_save(
        doc, item.url, site_config_, site_output_dir_);

    if (result.success) {
        models::PageDBEntry entry;
        entry.status = models::PageStatus::Success;
        entry.normalized_url = normalized;
        entry.content_hash = docscraper::utils::content_hash(result.markdown);
        entry.local_file_path = result.saved_file_path;
        entry.token_count = result.token_count;
        store_.update_page_status(normalized, entry);
        pages_processed_++;
    } else {
        models::PageDBEntry entry;
        entry.status = models::PageStatus::Failure;
        entry.error_type = models::ErrorType::ContentEmpty;
        entry.error_message = result.error;
        entry.normalized_url = normalized;
        store_.update_page_status(normalized, entry);
    }

    // Enqueue extracted links
    for (const auto& link : result.extracted_links) {
        auto resolved = parse::URLNormalizer::resolve(item.url, link);
        if (!resolved.has_value()) continue;
        if (!parse::URLNormalizer::is_in_scope(*resolved, site_config_.allowed_domain,
                                               site_config_.allowed_path_prefix)) {
            continue;
        }
        queue_.push(models::WorkItem(*resolved, item.depth + 1));
    }
}

} // namespace docscraper::crawler
