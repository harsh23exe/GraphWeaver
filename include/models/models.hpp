// models.hpp - Core Data Models
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace docscraper::models {

using json = nlohmann::json;
using TimePoint = std::chrono::system_clock::time_point;

// Page processing status
enum class PageStatus {
    Unknown,
    Pending,
    InProgress,
    Success,
    Failure,
    NotFound,
    OutOfScope,
    RobotsDisallowed
};

// Image processing status
enum class ImageStatus {
    Unknown,
    Pending,
    InProgress,
    Success,
    Failure,
    Skipped,
    TooLarge,
    InvalidDomain
};

// Error types for tracking failure reasons
enum class ErrorType {
    None,
    NetworkError,
    TimeoutError,
    HTTPError,
    ParseError,
    SelectorNotFound,
    ContentEmpty,
    IOError,
    RateLimited,
    RobotsDisallowed,
    OutOfScope,
    MaxRetriesExceeded,
    Unknown
};

// Convert enums to/from strings for JSON serialization
std::string to_string(PageStatus status);
PageStatus page_status_from_string(const std::string& str);

std::string to_string(ImageStatus status);
ImageStatus image_status_from_string(const std::string& str);

std::string to_string(ErrorType error);
ErrorType error_type_from_string(const std::string& str);

// Work item for the priority queue
struct WorkItem {
    std::string url;
    std::string normalized_url;
    int depth;
    int priority;  // Lower = higher priority
    TimePoint discovered_at;
    std::optional<std::string> referrer;  // URL that linked to this page
    
    WorkItem() : depth(0), priority(0), discovered_at(std::chrono::system_clock::now()) {}
    
    WorkItem(std::string u, int d)
        : url(std::move(u))
        , depth(d)
        , priority(d)  // Default priority equals depth (BFS)
        , discovered_at(std::chrono::system_clock::now())
    {}
    
    WorkItem(std::string u, int d, int p)
        : url(std::move(u))
        , depth(d)
        , priority(p)
        , discovered_at(std::chrono::system_clock::now())
    {}
    
    // Comparison for priority queue (min-heap)
    bool operator<(const WorkItem& other) const {
        return priority > other.priority;  // Lower priority value = higher priority
    }
    
    bool operator>(const WorkItem& other) const {
        return priority < other.priority;
    }
};

// Database entry for a crawled page
struct PageDBEntry {
    PageStatus status = PageStatus::Pending;
    ErrorType error_type = ErrorType::None;
    std::string error_message;
    TimePoint created_at;
    TimePoint processed_at;
    TimePoint last_attempt;
    int depth = 0;
    int attempt_count = 0;
    std::string content_hash;  // SHA256 of content for incremental crawling
    std::string normalized_url;
    std::string final_url;  // URL after redirects
    std::string local_file_path;
    int token_count = 0;
    
    // JSON serialization
    std::string to_json() const;
    static PageDBEntry from_json(const std::string& json_str);
    
    // Helper to update status
    void mark_success(const std::string& hash, const std::string& file_path, int tokens);
    void mark_failure(ErrorType type, const std::string& message);
    void mark_in_progress();
};

// Database entry for a downloaded image
struct ImageDBEntry {
    ImageStatus status = ImageStatus::Pending;
    ErrorType error_type = ErrorType::None;
    std::string error_message;
    TimePoint created_at;
    TimePoint processed_at;
    TimePoint last_attempt;
    int attempt_count = 0;
    std::string original_url;
    std::string local_path;
    std::string caption;  // Alt text
    int64_t file_size = 0;
    std::string content_type;
    
    // JSON serialization
    std::string to_json() const;
    static ImageDBEntry from_json(const std::string& json_str);
    
    // Helper methods
    void mark_success(const std::string& path, int64_t size, const std::string& type);
    void mark_failure(ErrorType type, const std::string& message);
    void mark_skipped(ImageStatus skip_reason);
};

// Metadata for a single crawled page (for YAML output)
struct PageMetadata {
    std::string original_url;
    std::string normalized_url;
    std::string final_url;
    std::string local_file_path;
    std::string title;
    int depth = 0;
    TimePoint processed_at;
    std::string content_hash;
    int image_count = 0;
    int link_count = 0;
    int token_count = 0;
    std::vector<std::string> headings;  // H1, H2, H3 headings
    
    json to_json() const;
};

// Metadata for a complete crawl session
struct CrawlMetadata {
    std::string site_key;
    std::string allowed_domain;
    TimePoint crawl_start_time;
    TimePoint crawl_end_time;
    int total_pages_saved = 0;
    int total_pages_failed = 0;
    int total_images_saved = 0;
    int total_images_skipped = 0;
    std::vector<PageMetadata> pages;
    
    json to_json() const;
    std::string to_yaml() const;
};

// JSONL format for RAG pipelines (per-page)
struct PageJSONL {
    std::string url;
    std::string title;
    std::string content;  // Full markdown content
    std::vector<std::string> headings;
    std::vector<std::string> links;
    std::vector<std::string> images;
    int token_count = 0;
    TimePoint crawled_at;
    
    std::string to_jsonl() const;  // Single line JSON
};

// JSONL format for RAG pipelines (per-chunk)
struct ChunkJSONL {
    std::string url;
    int chunk_index = 0;
    std::string content;
    std::vector<std::string> heading_hierarchy;
    int token_count = 0;
    
    std::string to_jsonl() const;
};

// HTTP fetch result
struct FetchResult {
    int status_code = 0;
    std::string body;
    std::string final_url;  // After redirects
    std::string content_type;
    std::string error;
    int64_t response_time_ms = 0;
    bool success = false;
    bool is_redirect = false;
    
    bool is_html() const;
    bool is_retryable() const;
};

// Robots.txt rule
struct RobotsRule {
    std::string user_agent;
    std::vector<std::string> disallow;
    std::vector<std::string> allow;
    std::optional<int> crawl_delay;  // In seconds
};

// Framework detection result
struct FrameworkDetection {
    std::string framework_name;
    std::string content_selector;
    float confidence = 0.0f;
    bool use_readability_fallback = false;
};

// Content extraction result
struct ExtractionResult {
    std::string title;
    std::string content_html;
    std::string content_markdown;
    std::vector<std::string> extracted_links;
    std::vector<std::string> image_urls;
    std::string content_hash;
    int token_count = 0;
    bool success = false;
    std::string error;
};

// LLM guidance suggestion (for LLM-guided crawling)
struct URLSuggestion {
    std::string url;
    float relevance_score = 0.0f;
    std::string reasoning;
    int priority_boost = 0;
};

// Structured data extraction result (for schema-driven JSON)
struct StructuredData {
    std::string url;
    std::string title;
    std::string content_type;  // tutorial, api_reference, guide, etc.
    std::string summary;
    TimePoint extracted_at;
    
    std::vector<json> headings;     // {level, text}
    std::vector<json> code_blocks;  // {language, code}
    std::vector<json> api_endpoints; // {method, path, description}
    std::vector<json> parameters;   // {name, type, required, description}
    std::vector<std::string> examples;
    
    json to_json() const;
    bool validate_against_schema(const std::string& schema_path) const;
};

} // namespace docscraper::models

// JSON serialization support for nlohmann/json
namespace nlohmann {

template<>
struct adl_serializer<docscraper::models::TimePoint> {
    static void to_json(json& j, const docscraper::models::TimePoint& tp);
    static void from_json(const json& j, docscraper::models::TimePoint& tp);
};

} // namespace nlohmann
