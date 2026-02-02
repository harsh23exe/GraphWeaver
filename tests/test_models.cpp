// test_models.cpp - Unit tests for data models
#include <gtest/gtest.h>
#include "models/models.hpp"

using namespace docscraper::models;

// ============================================================================
// WorkItem Tests
// ============================================================================

TEST(WorkItemTest, DefaultConstructor) {
    WorkItem item;
    EXPECT_EQ(item.depth, 0);
    EXPECT_EQ(item.priority, 0);
    EXPECT_TRUE(item.url.empty());
}

TEST(WorkItemTest, ConstructorWithUrlAndDepth) {
    WorkItem item("https://example.com", 3);
    EXPECT_EQ(item.url, "https://example.com");
    EXPECT_EQ(item.depth, 3);
    EXPECT_EQ(item.priority, 3);  // Default priority equals depth
}

TEST(WorkItemTest, ConstructorWithPriority) {
    WorkItem item("https://example.com", 3, 1);  // depth=3, priority=1
    EXPECT_EQ(item.depth, 3);
    EXPECT_EQ(item.priority, 1);
}

TEST(WorkItemTest, Comparison) {
    WorkItem high_priority("url1", 1);   // priority 1
    WorkItem low_priority("url2", 5);    // priority 5
    
    // Lower priority value = higher priority in queue
    EXPECT_TRUE(high_priority > low_priority);
    EXPECT_TRUE(low_priority < high_priority);
}

// ============================================================================
// PageStatus Enum Tests
// ============================================================================

TEST(PageStatusTest, ToStringConversion) {
    EXPECT_EQ(to_string(PageStatus::Pending), "pending");
    EXPECT_EQ(to_string(PageStatus::Success), "success");
    EXPECT_EQ(to_string(PageStatus::Failure), "failure");
    EXPECT_EQ(to_string(PageStatus::NotFound), "not_found");
}

TEST(PageStatusTest, FromStringConversion) {
    EXPECT_EQ(page_status_from_string("pending"), PageStatus::Pending);
    EXPECT_EQ(page_status_from_string("success"), PageStatus::Success);
    EXPECT_EQ(page_status_from_string("failure"), PageStatus::Failure);
    EXPECT_EQ(page_status_from_string("invalid"), PageStatus::Unknown);
}

// ============================================================================
// PageDBEntry Tests
// ============================================================================

TEST(PageDBEntryTest, JsonSerialization) {
    PageDBEntry entry;
    entry.status = PageStatus::Success;
    entry.depth = 2;
    entry.content_hash = "abc123";
    entry.normalized_url = "https://example.com/page";
    entry.token_count = 500;
    
    std::string json_str = entry.to_json();
    EXPECT_FALSE(json_str.empty());
    
    // Parse it back
    PageDBEntry parsed = PageDBEntry::from_json(json_str);
    EXPECT_EQ(parsed.status, PageStatus::Success);
    EXPECT_EQ(parsed.depth, 2);
    EXPECT_EQ(parsed.content_hash, "abc123");
    EXPECT_EQ(parsed.normalized_url, "https://example.com/page");
    EXPECT_EQ(parsed.token_count, 500);
}

TEST(PageDBEntryTest, MarkSuccess) {
    PageDBEntry entry;
    entry.status = PageStatus::Pending;
    
    entry.mark_success("hash123", "/path/to/file.md", 1000);
    
    EXPECT_EQ(entry.status, PageStatus::Success);
    EXPECT_EQ(entry.content_hash, "hash123");
    EXPECT_EQ(entry.local_file_path, "/path/to/file.md");
    EXPECT_EQ(entry.token_count, 1000);
    EXPECT_EQ(entry.error_type, ErrorType::None);
}

TEST(PageDBEntryTest, MarkFailure) {
    PageDBEntry entry;
    entry.status = PageStatus::Pending;
    
    entry.mark_failure(ErrorType::NetworkError, "Connection timeout");
    
    EXPECT_EQ(entry.status, PageStatus::Failure);
    EXPECT_EQ(entry.error_type, ErrorType::NetworkError);
    EXPECT_EQ(entry.error_message, "Connection timeout");
    EXPECT_EQ(entry.attempt_count, 1);
}

// ============================================================================
// ImageDBEntry Tests
// ============================================================================

TEST(ImageDBEntryTest, JsonSerialization) {
    ImageDBEntry entry;
    entry.status = ImageStatus::Success;
    entry.original_url = "https://example.com/image.png";
    entry.local_path = "/images/image.png";
    entry.file_size = 12345;
    entry.content_type = "image/png";
    
    std::string json_str = entry.to_json();
    EXPECT_FALSE(json_str.empty());
    
    ImageDBEntry parsed = ImageDBEntry::from_json(json_str);
    EXPECT_EQ(parsed.status, ImageStatus::Success);
    EXPECT_EQ(parsed.original_url, "https://example.com/image.png");
    EXPECT_EQ(parsed.local_path, "/images/image.png");
    EXPECT_EQ(parsed.file_size, 12345);
}

TEST(ImageDBEntryTest, MarkSkipped) {
    ImageDBEntry entry;
    entry.mark_skipped(ImageStatus::TooLarge);
    
    EXPECT_EQ(entry.status, ImageStatus::TooLarge);
}

// ============================================================================
// FetchResult Tests
// ============================================================================

TEST(FetchResultTest, IsHtml) {
    FetchResult result;
    
    result.content_type = "text/html; charset=utf-8";
    EXPECT_TRUE(result.is_html());
    
    result.content_type = "application/json";
    EXPECT_FALSE(result.is_html());
}

TEST(FetchResultTest, IsRetryable) {
    FetchResult result;
    
    result.status_code = 500;
    EXPECT_TRUE(result.is_retryable());
    
    result.status_code = 503;
    EXPECT_TRUE(result.is_retryable());
    
    result.status_code = 429;
    EXPECT_TRUE(result.is_retryable());
    
    result.status_code = 0;  // Network error
    EXPECT_TRUE(result.is_retryable());
    
    result.status_code = 404;
    EXPECT_FALSE(result.is_retryable());
    
    result.status_code = 200;
    EXPECT_FALSE(result.is_retryable());
}

// ============================================================================
// PageJSONL Tests
// ============================================================================

TEST(PageJSONLTest, ToJsonl) {
    PageJSONL page;
    page.url = "https://example.com/page";
    page.title = "Test Page";
    page.content = "# Hello\n\nWorld";
    page.token_count = 50;
    
    std::string jsonl = page.to_jsonl();
    EXPECT_FALSE(jsonl.empty());
    
    // Should be a single line (no newlines in output)
    EXPECT_EQ(jsonl.find('\n'), std::string::npos);
    
    // Should contain our data
    EXPECT_NE(jsonl.find("Test Page"), std::string::npos);
    EXPECT_NE(jsonl.find("example.com"), std::string::npos);
}

// ============================================================================
// CrawlMetadata Tests
// ============================================================================

TEST(CrawlMetadataTest, ToYaml) {
    CrawlMetadata metadata;
    metadata.site_key = "test_site";
    metadata.allowed_domain = "example.com";
    metadata.total_pages_saved = 100;
    metadata.total_pages_failed = 5;
    
    std::string yaml = metadata.to_yaml();
    EXPECT_FALSE(yaml.empty());
    EXPECT_NE(yaml.find("test_site"), std::string::npos);
    EXPECT_NE(yaml.find("example.com"), std::string::npos);
    EXPECT_NE(yaml.find("100"), std::string::npos);
}

// ============================================================================
// ErrorType Tests
// ============================================================================

TEST(ErrorTypeTest, RoundTrip) {
    std::vector<ErrorType> types = {
        ErrorType::None,
        ErrorType::NetworkError,
        ErrorType::TimeoutError,
        ErrorType::HTTPError,
        ErrorType::ParseError,
        ErrorType::RateLimited,
        ErrorType::MaxRetriesExceeded
    };
    
    for (auto type : types) {
        std::string str = to_string(type);
        ErrorType parsed = error_type_from_string(str);
        EXPECT_EQ(type, parsed) << "Failed for: " << str;
    }
}
