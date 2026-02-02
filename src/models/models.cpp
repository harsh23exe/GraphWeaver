// models.cpp - Core Data Models Implementation
// LLM Documentation Scraper - C++ Implementation

#include "models/models.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace docscraper::models {

// ============================================================================
// Enum to/from string conversions
// ============================================================================

std::string to_string(PageStatus status) {
    switch (status) {
        case PageStatus::Unknown: return "unknown";
        case PageStatus::Pending: return "pending";
        case PageStatus::InProgress: return "in_progress";
        case PageStatus::Success: return "success";
        case PageStatus::Failure: return "failure";
        case PageStatus::NotFound: return "not_found";
        case PageStatus::OutOfScope: return "out_of_scope";
        case PageStatus::RobotsDisallowed: return "robots_disallowed";
    }
    return "unknown";
}

PageStatus page_status_from_string(const std::string& str) {
    if (str == "pending") return PageStatus::Pending;
    if (str == "in_progress") return PageStatus::InProgress;
    if (str == "success") return PageStatus::Success;
    if (str == "failure") return PageStatus::Failure;
    if (str == "not_found") return PageStatus::NotFound;
    if (str == "out_of_scope") return PageStatus::OutOfScope;
    if (str == "robots_disallowed") return PageStatus::RobotsDisallowed;
    return PageStatus::Unknown;
}

std::string to_string(ImageStatus status) {
    switch (status) {
        case ImageStatus::Unknown: return "unknown";
        case ImageStatus::Pending: return "pending";
        case ImageStatus::InProgress: return "in_progress";
        case ImageStatus::Success: return "success";
        case ImageStatus::Failure: return "failure";
        case ImageStatus::Skipped: return "skipped";
        case ImageStatus::TooLarge: return "too_large";
        case ImageStatus::InvalidDomain: return "invalid_domain";
    }
    return "unknown";
}

ImageStatus image_status_from_string(const std::string& str) {
    if (str == "pending") return ImageStatus::Pending;
    if (str == "in_progress") return ImageStatus::InProgress;
    if (str == "success") return ImageStatus::Success;
    if (str == "failure") return ImageStatus::Failure;
    if (str == "skipped") return ImageStatus::Skipped;
    if (str == "too_large") return ImageStatus::TooLarge;
    if (str == "invalid_domain") return ImageStatus::InvalidDomain;
    return ImageStatus::Unknown;
}

std::string to_string(ErrorType error) {
    switch (error) {
        case ErrorType::None: return "none";
        case ErrorType::NetworkError: return "network_error";
        case ErrorType::TimeoutError: return "timeout_error";
        case ErrorType::HTTPError: return "http_error";
        case ErrorType::ParseError: return "parse_error";
        case ErrorType::SelectorNotFound: return "selector_not_found";
        case ErrorType::ContentEmpty: return "content_empty";
        case ErrorType::IOError: return "io_error";
        case ErrorType::RateLimited: return "rate_limited";
        case ErrorType::RobotsDisallowed: return "robots_disallowed";
        case ErrorType::OutOfScope: return "out_of_scope";
        case ErrorType::MaxRetriesExceeded: return "max_retries_exceeded";
        case ErrorType::Unknown: return "unknown";
    }
    return "unknown";
}

ErrorType error_type_from_string(const std::string& str) {
    if (str == "none") return ErrorType::None;
    if (str == "network_error") return ErrorType::NetworkError;
    if (str == "timeout_error") return ErrorType::TimeoutError;
    if (str == "http_error") return ErrorType::HTTPError;
    if (str == "parse_error") return ErrorType::ParseError;
    if (str == "selector_not_found") return ErrorType::SelectorNotFound;
    if (str == "content_empty") return ErrorType::ContentEmpty;
    if (str == "io_error") return ErrorType::IOError;
    if (str == "rate_limited") return ErrorType::RateLimited;
    if (str == "robots_disallowed") return ErrorType::RobotsDisallowed;
    if (str == "out_of_scope") return ErrorType::OutOfScope;
    if (str == "max_retries_exceeded") return ErrorType::MaxRetriesExceeded;
    return ErrorType::Unknown;
}

// ============================================================================
// TimePoint serialization helpers
// ============================================================================

static std::string timepoint_to_iso8601(const TimePoint& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::tm tm_val{};
    gmtime_r(&time_t_val, &tm_val);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

static TimePoint iso8601_to_timepoint(const std::string& str) {
    std::tm tm_val{};
    std::istringstream iss(str);
    iss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    
    auto time_t_val = timegm(&tm_val);
    auto tp = std::chrono::system_clock::from_time_t(time_t_val);
    
    // Parse milliseconds if present
    if (str.length() > 19 && str[19] == '.') {
        auto ms_str = str.substr(20, 3);
        if (!ms_str.empty()) {
            int ms = std::stoi(ms_str);
            tp += std::chrono::milliseconds(ms);
        }
    }
    
    return tp;
}

// ============================================================================
// PageDBEntry implementation
// ============================================================================

std::string PageDBEntry::to_json() const {
    json j;
    j["status"] = to_string(status);
    j["error_type"] = to_string(error_type);
    j["error_message"] = error_message;
    j["created_at"] = timepoint_to_iso8601(created_at);
    j["processed_at"] = timepoint_to_iso8601(processed_at);
    j["last_attempt"] = timepoint_to_iso8601(last_attempt);
    j["depth"] = depth;
    j["attempt_count"] = attempt_count;
    j["content_hash"] = content_hash;
    j["normalized_url"] = normalized_url;
    j["final_url"] = final_url;
    j["local_file_path"] = local_file_path;
    j["token_count"] = token_count;
    return j.dump();
}

PageDBEntry PageDBEntry::from_json(const std::string& json_str) {
    PageDBEntry entry;
    try {
        json j = json::parse(json_str);
        entry.status = page_status_from_string(j.value("status", "unknown"));
        entry.error_type = error_type_from_string(j.value("error_type", "none"));
        entry.error_message = j.value("error_message", "");
        entry.created_at = iso8601_to_timepoint(j.value("created_at", ""));
        entry.processed_at = iso8601_to_timepoint(j.value("processed_at", ""));
        entry.last_attempt = iso8601_to_timepoint(j.value("last_attempt", ""));
        entry.depth = j.value("depth", 0);
        entry.attempt_count = j.value("attempt_count", 0);
        entry.content_hash = j.value("content_hash", "");
        entry.normalized_url = j.value("normalized_url", "");
        entry.final_url = j.value("final_url", "");
        entry.local_file_path = j.value("local_file_path", "");
        entry.token_count = j.value("token_count", 0);
    } catch (const json::exception&) {
        // Return default entry on parse error
    }
    return entry;
}

void PageDBEntry::mark_success(const std::string& hash, const std::string& file_path, int tokens) {
    status = PageStatus::Success;
    error_type = ErrorType::None;
    error_message.clear();
    processed_at = std::chrono::system_clock::now();
    content_hash = hash;
    local_file_path = file_path;
    token_count = tokens;
}

void PageDBEntry::mark_failure(ErrorType type, const std::string& message) {
    status = PageStatus::Failure;
    error_type = type;
    error_message = message;
    last_attempt = std::chrono::system_clock::now();
    ++attempt_count;
}

void PageDBEntry::mark_in_progress() {
    status = PageStatus::InProgress;
    last_attempt = std::chrono::system_clock::now();
}

// ============================================================================
// ImageDBEntry implementation
// ============================================================================

std::string ImageDBEntry::to_json() const {
    json j;
    j["status"] = to_string(status);
    j["error_type"] = to_string(error_type);
    j["error_message"] = error_message;
    j["created_at"] = timepoint_to_iso8601(created_at);
    j["processed_at"] = timepoint_to_iso8601(processed_at);
    j["last_attempt"] = timepoint_to_iso8601(last_attempt);
    j["attempt_count"] = attempt_count;
    j["original_url"] = original_url;
    j["local_path"] = local_path;
    j["caption"] = caption;
    j["file_size"] = file_size;
    j["content_type"] = content_type;
    return j.dump();
}

ImageDBEntry ImageDBEntry::from_json(const std::string& json_str) {
    ImageDBEntry entry;
    try {
        json j = json::parse(json_str);
        entry.status = image_status_from_string(j.value("status", "unknown"));
        entry.error_type = error_type_from_string(j.value("error_type", "none"));
        entry.error_message = j.value("error_message", "");
        entry.created_at = iso8601_to_timepoint(j.value("created_at", ""));
        entry.processed_at = iso8601_to_timepoint(j.value("processed_at", ""));
        entry.last_attempt = iso8601_to_timepoint(j.value("last_attempt", ""));
        entry.attempt_count = j.value("attempt_count", 0);
        entry.original_url = j.value("original_url", "");
        entry.local_path = j.value("local_path", "");
        entry.caption = j.value("caption", "");
        entry.file_size = j.value("file_size", 0);
        entry.content_type = j.value("content_type", "");
    } catch (const json::exception&) {
        // Return default entry on parse error
    }
    return entry;
}

void ImageDBEntry::mark_success(const std::string& path, int64_t size, const std::string& type) {
    status = ImageStatus::Success;
    error_type = ErrorType::None;
    error_message.clear();
    processed_at = std::chrono::system_clock::now();
    local_path = path;
    file_size = size;
    content_type = type;
}

void ImageDBEntry::mark_failure(ErrorType type, const std::string& message) {
    status = ImageStatus::Failure;
    error_type = type;
    error_message = message;
    last_attempt = std::chrono::system_clock::now();
    ++attempt_count;
}

void ImageDBEntry::mark_skipped(ImageStatus skip_reason) {
    status = skip_reason;
    processed_at = std::chrono::system_clock::now();
}

// ============================================================================
// PageMetadata implementation
// ============================================================================

json PageMetadata::to_json() const {
    json j;
    j["original_url"] = original_url;
    j["normalized_url"] = normalized_url;
    j["final_url"] = final_url;
    j["local_file_path"] = local_file_path;
    j["title"] = title;
    j["depth"] = depth;
    j["processed_at"] = timepoint_to_iso8601(processed_at);
    j["content_hash"] = content_hash;
    j["image_count"] = image_count;
    j["link_count"] = link_count;
    j["token_count"] = token_count;
    j["headings"] = headings;
    return j;
}

// ============================================================================
// CrawlMetadata implementation
// ============================================================================

json CrawlMetadata::to_json() const {
    json j;
    j["site_key"] = site_key;
    j["allowed_domain"] = allowed_domain;
    j["crawl_start_time"] = timepoint_to_iso8601(crawl_start_time);
    j["crawl_end_time"] = timepoint_to_iso8601(crawl_end_time);
    j["total_pages_saved"] = total_pages_saved;
    j["total_pages_failed"] = total_pages_failed;
    j["total_images_saved"] = total_images_saved;
    j["total_images_skipped"] = total_images_skipped;
    
    json pages_json = json::array();
    for (const auto& page : pages) {
        pages_json.push_back(page.to_json());
    }
    j["pages"] = pages_json;
    
    return j;
}

std::string CrawlMetadata::to_yaml() const {
    std::ostringstream oss;
    oss << "# Crawl Metadata\n";
    oss << "site_key: " << site_key << "\n";
    oss << "allowed_domain: " << allowed_domain << "\n";
    oss << "crawl_start_time: " << timepoint_to_iso8601(crawl_start_time) << "\n";
    oss << "crawl_end_time: " << timepoint_to_iso8601(crawl_end_time) << "\n";
    oss << "total_pages_saved: " << total_pages_saved << "\n";
    oss << "total_pages_failed: " << total_pages_failed << "\n";
    oss << "total_images_saved: " << total_images_saved << "\n";
    oss << "total_images_skipped: " << total_images_skipped << "\n";
    oss << "pages:\n";
    for (const auto& page : pages) {
        oss << "  - url: " << page.original_url << "\n";
        oss << "    title: \"" << page.title << "\"\n";
        oss << "    file: " << page.local_file_path << "\n";
        oss << "    depth: " << page.depth << "\n";
        oss << "    tokens: " << page.token_count << "\n";
    }
    return oss.str();
}

// ============================================================================
// PageJSONL implementation
// ============================================================================

std::string PageJSONL::to_jsonl() const {
    json j;
    j["url"] = url;
    j["title"] = title;
    j["content"] = content;
    j["headings"] = headings;
    j["links"] = links;
    j["images"] = images;
    j["token_count"] = token_count;
    j["crawled_at"] = timepoint_to_iso8601(crawled_at);
    return j.dump();  // Single line, no pretty print
}

// ============================================================================
// ChunkJSONL implementation
// ============================================================================

std::string ChunkJSONL::to_jsonl() const {
    json j;
    j["url"] = url;
    j["chunk_index"] = chunk_index;
    j["content"] = content;
    j["heading_hierarchy"] = heading_hierarchy;
    j["token_count"] = token_count;
    return j.dump();
}

// ============================================================================
// FetchResult implementation
// ============================================================================

bool FetchResult::is_html() const {
    return content_type.find("text/html") != std::string::npos;
}

bool FetchResult::is_retryable() const {
    // Retry on server errors and rate limiting
    if (status_code >= 500 && status_code < 600) return true;
    if (status_code == 429) return true;  // Too Many Requests
    if (status_code == 0) return true;    // Network error (no response)
    return false;
}

// ============================================================================
// StructuredData implementation
// ============================================================================

json StructuredData::to_json() const {
    json j;
    j["url"] = url;
    j["title"] = title;
    j["content_type"] = content_type;
    j["summary"] = summary;
    j["extracted_at"] = timepoint_to_iso8601(extracted_at);
    j["headings"] = headings;
    j["code_blocks"] = code_blocks;
    j["api_endpoints"] = api_endpoints;
    j["parameters"] = parameters;
    j["examples"] = examples;
    return j;
}

bool StructuredData::validate_against_schema(const std::string& /*schema_path*/) const {
    // TODO: Implement JSON schema validation
    // For now, just check required fields are present
    return !url.empty() && !title.empty();
}

} // namespace docscraper::models

// ============================================================================
// nlohmann/json TimePoint serialization
// ============================================================================

namespace nlohmann {

void adl_serializer<docscraper::models::TimePoint>::to_json(
    json& j, const docscraper::models::TimePoint& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::tm tm_val{};
    gmtime_r(&time_t_val, &tm_val);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    j = oss.str();
}

void adl_serializer<docscraper::models::TimePoint>::from_json(
    const json& j, docscraper::models::TimePoint& tp) {
    std::string str = j.get<std::string>();
    
    std::tm tm_val{};
    std::istringstream iss(str);
    iss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    
    auto time_t_val = timegm(&tm_val);
    tp = std::chrono::system_clock::from_time_t(time_t_val);
    
    if (str.length() > 19 && str[19] == '.') {
        auto ms_str = str.substr(20, 3);
        if (!ms_str.empty()) {
            int ms = std::stoi(ms_str);
            tp += std::chrono::milliseconds(ms);
        }
    }
}

} // namespace nlohmann
