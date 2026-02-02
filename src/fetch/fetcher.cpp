// fetcher.cpp - HTTP Fetcher Implementation
// LLM Documentation Scraper - C++ Implementation

#include "fetch/fetcher.hpp"
#include "parse/normalizer.hpp"
#include <chrono>
#include <thread>

namespace docscraper::fetch {

Fetcher::Fetcher(const crawler::AppConfig& config)
    : config_(config) {}

std::unique_ptr<httplib::Client> Fetcher::build_client(const std::string& base_url) {
    auto client = std::make_unique<httplib::Client>(base_url.c_str());
    client->set_follow_location(config_.http_client_settings.follow_redirects);
    client->set_connection_timeout(
        std::chrono::duration_cast<std::chrono::seconds>(config_.http_client_settings.timeout).count());
    client->set_read_timeout(
        std::chrono::duration_cast<std::chrono::seconds>(config_.http_client_settings.timeout).count());
    client->set_write_timeout(
        std::chrono::duration_cast<std::chrono::seconds>(config_.http_client_settings.timeout).count());
    client->set_keep_alive(true);
    return client;
}

std::string Fetcher::extract_base_url(const std::string& url) const {
    auto parsed = docscraper::parse::URLNormalizer::parse(url);
    if (!parsed) return url;
    std::string base = parsed->scheme + "://" + parsed->host;
    if (parsed->port > 0 && !parsed->is_default_port()) {
        base += ":" + std::to_string(parsed->port);
    }
    return base;
}

bool Fetcher::is_retryable_status(int status_code) const {
    if (status_code == 0) return true;  // Network error
    if (status_code == 429) return true;  // Too Many Requests
    if (status_code >= 500 && status_code < 600) return true;
    return false;
}

std::chrono::milliseconds Fetcher::calculate_backoff(int attempt) const {
    // Exponential backoff with jitter
    auto base = config_.initial_retry_delay.count() * (1LL << (attempt - 1));
    auto max_delay = config_.max_retry_delay.count();
    if (base > max_delay) base = max_delay;

    // Jitter +/-10%
    int64_t jitter = static_cast<int64_t>(base * 0.1);
    int64_t actual = base;
    if (jitter > 0) {
        actual = base + (rand() % (2 * jitter + 1) - jitter);
    }

    return std::chrono::milliseconds(actual);
}

models::FetchResult Fetcher::fetch_once(const std::string& url) {
    models::FetchResult result;
    result.final_url = url;

    auto parsed = docscraper::parse::URLNormalizer::parse(url);
    if (!parsed) {
        result.success = false;
        result.error = "Invalid URL";
        return result;
    }

    std::string base_url = extract_base_url(url);
    auto client = build_client(base_url);

    httplib::Headers headers = {
        {"User-Agent", config_.http_client_settings.user_agent}
    };

    auto path = parsed->path;
    if (!parsed->query.empty()) {
        path += "?" + parsed->query;
    }

    auto start = std::chrono::steady_clock::now();
    auto res = client->Get(path.c_str(), headers);
    auto end = std::chrono::steady_clock::now();

    result.response_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (!res) {
        result.success = false;
        result.status_code = 0;
        result.error = "Network error or timeout";
        return result;
    }

    result.status_code = res->status;
    result.body = res->body;
    result.success = (res->status >= 200 && res->status < 300);
    result.content_type = res->get_header_value("Content-Type");

    // If redirected, attempt to read Location header
    if (res->status >= 300 && res->status < 400) {
        auto loc = res->get_header_value("Location");
        if (!loc.empty()) {
            result.is_redirect = true;
            result.final_url = loc;
        }
    }

    return result;
}

models::FetchResult Fetcher::fetch_with_retry(const std::string& url) {
    models::FetchResult last_result;

    for (int attempt = 1; attempt <= config_.max_retries + 1; ++attempt) {
        last_result = fetch_once(url);

        if (last_result.success || !is_retryable_status(last_result.status_code)) {
            return last_result;
        }

        if (attempt <= config_.max_retries) {
            auto delay = calculate_backoff(attempt);
            std::this_thread::sleep_for(delay);
        }
    }

    return last_result;
}

} // namespace docscraper::fetch
