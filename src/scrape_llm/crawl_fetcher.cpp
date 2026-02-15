#include "scrape_llm/crawl_fetcher.hpp"
#include "scrape_llm/ssrf_guard.hpp"
#include "parse/normalizer.hpp"
#include "utils/hash.hpp"
#include <httplib.h>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace scrapellm {

namespace fs = std::filesystem;

CrawlFetcher::CrawlFetcher(const RunConfig& config)
    : config_(config)
    , rate_limiter_(config.rate_limit_delay_ms())
    , user_agent_("scrape-llm/1.0 (+https://github.com/GraphWeaver)")
{
    cache_dir_ = config.out_dir + "/cache/pages";
}

std::string CrawlFetcher::cache_path_for(const std::string& normalized_url) const {
    return cache_dir_ + "/" + docscraper::utils::sha256_hash(normalized_url) + ".html";
}

bool CrawlFetcher::load_from_cache(const std::string& normalized_url, std::string& out_html) const {
    std::string path = cache_path_for(normalized_url);
    std::ifstream f(path);
    if (!f) return false;
    out_html.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

void CrawlFetcher::save_to_cache(const std::string& normalized_url, const std::string& html) {
    fs::create_directories(cache_dir_);
    std::string path = cache_path_for(normalized_url);
    std::ofstream f(path);
    if (f) f << html;
}

std::string CrawlFetcher::fetch_robots(const std::string& base_url) {
    auto parsed = docscraper::parse::URLNormalizer::parse(base_url);
    if (!parsed) return "";
    robots_origin_ = parsed->scheme + "://" + parsed->host;
    if (parsed->port > 0 && !parsed->is_default_port())
        robots_origin_ += ":" + std::to_string(parsed->port);

    std::string robots_url = robots_origin_ + "/robots.txt";
    if (!url_allowed_ssrf(robots_url, config_.allow_private_network)) return "";
    rate_limiter_.wait_for_host(parsed->host);

    std::string path = "/robots.txt";
    httplib::Client client(robots_origin_.c_str());
    client.set_follow_location(true);
    client.set_connection_timeout(10);
    client.set_read_timeout(10);
    auto res = client.Get(path.c_str(), {{"User-Agent", user_agent_}});
    if (res && res->status == 200) {
        robots_.parse(res->body);
        return res->body;
    }
    return "";
}

bool CrawlFetcher::is_allowed_by_robots(const std::string& url) const {
    if (!config_.respect_robots) return true;
    auto parsed = docscraper::parse::URLNormalizer::parse(url);
    if (!parsed) return false;
    std::string path = parsed->path;
    if (!parsed->query.empty()) path += "?" + parsed->query;
    return robots_.is_allowed(path, "*");
}

bool CrawlFetcher::seen_add(const std::string& normalized_url) {
    auto it = seen_urls_.find(normalized_url);
    if (it != seen_urls_.end()) return false;
    seen_urls_.insert(normalized_url);
    return true;
}

std::optional<CrawlResult> CrawlFetcher::fetch(const std::string& url, int depth) {
    std::string normalized = docscraper::parse::URLNormalizer::normalize(url, false);
    if (!url_allowed_ssrf(normalized, config_.allow_private_network)) {
        CrawlResult r;
        r.url = url;
        r.success = false;
        r.error = "SSRF blocked";
        return r;
    }

    auto parsed = docscraper::parse::URLNormalizer::parse(normalized);
    if (!parsed) {
        CrawlResult r;
        r.url = url;
        r.success = false;
        r.error = "Invalid URL";
        return r;
    }

    std::string html;
    if (load_from_cache(normalized, html)) {
        CrawlResult r;
        r.url = url;
        r.normalized_url = normalized;
        r.depth = depth;
        r.html = std::move(html);
        r.final_url = normalized;
        r.success = true;
        return r;
    }

    rate_limiter_.wait_for_host(parsed->host);

    std::string base = parsed->scheme + "://" + parsed->host;
    if (parsed->port > 0 && !parsed->is_default_port())
        base += ":" + std::to_string(parsed->port);
    httplib::Client client(base.c_str());
    client.set_follow_location(true);
    client.set_connection_timeout(30);
    client.set_read_timeout(30);
    std::string path = parsed->path;
    if (!parsed->query.empty()) path += "?" + parsed->query;

    auto res = client.Get(path.c_str(), {{"User-Agent", user_agent_}});
    CrawlResult result;
    result.url = url;
    result.normalized_url = normalized;
    result.depth = depth;

    if (!res) {
        result.success = false;
        result.error = "Network error or timeout";
        return result;
    }
    if (res->status != 200) {
        result.success = false;
        result.error = "HTTP " + std::to_string(res->status);
        return result;
    }
    std::string content_type = res->get_header_value("Content-Type");
    if (content_type.find("text/html") == std::string::npos &&
        content_type.find("application/xhtml") == std::string::npos) {
        result.success = false;
        result.error = "Not HTML";
        return result;
    }
    result.html = res->body;
    result.final_url = normalized;
    result.success = true;
    save_to_cache(normalized, result.html);
    return result;
}

} // namespace scrapellm
