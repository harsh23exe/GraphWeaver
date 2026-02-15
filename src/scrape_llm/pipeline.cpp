#include "scrape_llm/pipeline.hpp"
#include "scrape_llm/crawl_fetcher.hpp"
#include "scrape_llm/content_extractor.hpp"
#include "scrape_llm/schema_infer.hpp"
#include "scrape_llm/relevance_router.hpp"
#include "scrape_llm/record_parser.hpp"
#include "scrape_llm/validator.hpp"
#include "scrape_llm/output_writers.hpp"
#include "scrape_llm/report_generator.hpp"
#include "scrape_llm/ssrf_guard.hpp"
#include "parse/html_parser.hpp"
#include "parse/normalizer.hpp"
#include <spdlog/spdlog.h>
#include <queue>
#include <chrono>
#include <set>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace scrapellm {

static std::string extract_origin(const std::string& url) {
    auto p = docscraper::parse::URLNormalizer::parse(url);
    if (!p) return "";
    std::string o = p->scheme + "://" + p->host;
    if (p->port > 0 && !p->is_default_port()) o += ":" + std::to_string(p->port);
    return o;
}

static std::string stable_json_hash(const nlohmann::json& j) {
    return nlohmann::json(j).dump();
}

int run_pipeline(const RunConfig& config) {
    RunReport report;

    if (!docscraper::parse::URLNormalizer::is_valid_http_url(config.url)) {
        spdlog::error("Invalid start URL: {}", config.url);
        return 1;
    }
    if (!url_allowed_ssrf(config.url, config.allow_private_network)) {
        spdlog::error("Start URL blocked by SSRF policy");
        return 1;
    }

    std::string base_origin = extract_origin(config.url);
    if (base_origin.empty()) {
        spdlog::error("Could not parse start URL");
        return 1;
    }

    std::string api_key_env = "GEMINI_API_KEY";
    std::string base_url = config.base_url.empty() ? "https://generativelanguage.googleapis.com/v1beta/openai/" : config.base_url;
    LlmClient llm(base_url, config.model, api_key_env);
    llm.set_max_tokens(4096);

    if (llm.get_api_key().empty()) {
        spdlog::error("GEMINI_API_KEY not set");
        return 1;
    }

    InferredSchema schema;
    std::string schema_warning;
    schema = schema_infer(llm, config.schema, schema_warning);
    if (!schema_warning.empty()) spdlog::warn("{}", schema_warning);

    nlohmann::json schema_to_save = {{"json_schema", schema.json_schema}, {"extraction_mode", schema.extraction_mode}, {"hints", schema.hints}};
    write_schema(config.out_dir, schema_to_save);

    CrawlFetcher fetcher(config);
    if (config.respect_robots)
        fetcher.fetch_robots(config.url);

    struct QueuedUrl { std::string url; int depth; };
    std::queue<QueuedUrl> queue;
    queue.push({config.url, 0});
    std::set<std::string> queued;
    queued.insert(docscraper::parse::URLNormalizer::normalize(config.url, false));

    auto t_crawl_start = std::chrono::steady_clock::now();
    std::vector<CrawlResult> crawled;
    std::vector<std::string> all_html;
    std::vector<std::string> all_urls;

    while (!queue.empty() && static_cast<int>(crawled.size()) < config.max_pages) {
        QueuedUrl qu = queue.front();
        queue.pop();
        auto res = fetcher.fetch(qu.url, qu.depth);
        if (!res) continue;
        if (!res->success) {
            report.errors.push_back(res->url + ": " + res->error);
            continue;
        }
        if (config.respect_robots && !fetcher.is_allowed_by_robots(res->url))
            continue;

        report.pages_visited.push_back(res->url);
        report.pages_crawled++;
        crawled.push_back(*res);
        all_html.push_back(res->html);
        all_urls.push_back(res->final_url);

        if (qu.depth >= config.max_depth) continue;
        docscraper::parse::HTMLDocument doc(res->html);
        auto links = extract_links_absolute(doc, res->final_url);
        for (const auto& link : links) {
            if (extract_origin(link) != base_origin) continue;
            std::string norm = docscraper::parse::URLNormalizer::normalize(link, false);
            if (!url_allowed_ssrf(norm, config.allow_private_network)) continue;
            if (queued.count(norm)) continue;
            queued.insert(norm);
            queue.push({link, qu.depth + 1});
        }
    }

    report.crawl_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_crawl_start);

    if (config.dry_run) {
        report.llm_ms = std::chrono::milliseconds(0);
        write_report(config.out_dir, report);
        spdlog::info("Dry run: crawled {} pages", report.pages_crawled);
        return 0;
    }

    std::vector<PageDigest> digests;
    for (size_t i = 0; i < all_urls.size(); ++i) {
        docscraper::parse::HTMLDocument doc(all_html[i]);
        ExtractedContent content = extract_content(doc, all_urls[i]);
        digests.push_back(make_digest(content, 1500));
    }

    std::vector<PageDigest> to_parse = select_pages_to_parse(llm, config.schema, digests, config.keep_pages);
    report.pages_kept = static_cast<int>(to_parse.size());

    std::vector<nlohmann::json> all_records;
    auto t_llm_start = std::chrono::steady_clock::now();

    for (const auto& d : to_parse) {
        size_t idx = 0;
        for (; idx < all_urls.size(); ++idx) if (all_urls[idx] == d.url) break;
        if (idx >= all_urls.size()) continue;
        docscraper::parse::HTMLDocument doc(all_html[idx]);
        ExtractedContent content = extract_content(doc, all_urls[idx]);
        auto records = parse_records(llm, schema, content);
        for (auto& rec : records) {
            ValidationResult vr = validate_record(rec, schema.json_schema);
            if (vr.valid) {
                all_records.push_back(std::move(rec));
                continue;
            }
            report.validation_failures++;
            report.repair_attempts++;
            auto repaired = repair_record(llm, rec, schema.json_schema, vr.error_message);
            if (repaired) {
                ValidationResult vr2 = validate_record(*repaired, schema.json_schema);
                if (vr2.valid) {
                    report.repair_successes++;
                    all_records.push_back(std::move(*repaired));
                } else {
                    report.errors.push_back("Repair still invalid: " + vr.error_message);
                }
            } else {
                report.errors.push_back("Repair failed: " + vr.error_message);
            }
        }
    }

    report.llm_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_llm_start);

    std::set<std::string> seen_hashes;
    std::vector<nlohmann::json> deduped;
    std::string dedupe_key;
    if (schema.hints.contains("dedupe_key")) {
        if (schema.hints["dedupe_key"].is_string())
            dedupe_key = schema.hints["dedupe_key"].get<std::string>();
        else if (schema.hints["dedupe_key"].is_array() && !schema.hints["dedupe_key"].empty())
            dedupe_key = schema.hints["dedupe_key"][0].get<std::string>();
    }
    for (auto& r : all_records) {
        std::string key;
        if (!dedupe_key.empty() && r.contains(dedupe_key))
            key = r[dedupe_key].dump();
        else
            key = stable_json_hash(r);
        if (seen_hashes.count(key)) continue;
        seen_hashes.insert(key);
        deduped.push_back(std::move(r));
    }

    report.records_emitted = static_cast<int>(deduped.size());
    write_outputs(config.out_dir, config.format, config.emit_csv, deduped);
    write_report(config.out_dir, report);

    spdlog::info("Done: {} pages crawled, {} kept, {} records", report.pages_crawled, report.pages_kept, report.records_emitted);
    return 0;
}

} // namespace scrapellm
