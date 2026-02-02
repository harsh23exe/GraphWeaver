// test_config.cpp - Unit tests for configuration parser
#include <gtest/gtest.h>
#include "crawler/config.hpp"

using namespace docscraper::crawler;

// ============================================================================
// Duration Parsing Tests
// ============================================================================

TEST(DurationParseTest, Milliseconds) {
    auto ms = parse_duration("500ms");
    EXPECT_EQ(ms.count(), 500);
}

TEST(DurationParseTest, Seconds) {
    auto ms = parse_duration("30s");
    EXPECT_EQ(ms.count(), 30000);
}

TEST(DurationParseTest, SecondsNoUnit) {
    auto ms = parse_duration("10");
    EXPECT_EQ(ms.count(), 10000);  // Default unit is seconds
}

TEST(DurationParseTest, Minutes) {
    auto ms = parse_duration("5m");
    EXPECT_EQ(ms.count(), 300000);
}

TEST(DurationParseTest, Hours) {
    auto ms = parse_duration("2h");
    EXPECT_EQ(ms.count(), 7200000);
}

TEST(DurationParseTest, InvalidUnit) {
    EXPECT_THROW(parse_duration("10xyz"), ConfigError);
}

TEST(DurationParseTest, EmptyString) {
    auto ms = parse_duration("");
    EXPECT_EQ(ms.count(), 0);
}

TEST(DurationToStringTest, Milliseconds) {
    EXPECT_EQ(duration_to_string(std::chrono::milliseconds(500)), "500ms");
}

TEST(DurationToStringTest, Seconds) {
    EXPECT_EQ(duration_to_string(std::chrono::milliseconds(30000)), "30s");
}

// ============================================================================
// SiteConfig Tests
// ============================================================================

TEST(SiteConfigTest, LoadFromYaml) {
    std::string yaml_content = R"(
start_urls:
  - https://example.com/docs
allowed_domain: example.com
allowed_path_prefix: /docs
content_selector: main
max_depth: 3
delay_per_host: 500ms
skip_images: false
)";
    
    YAML::Node node = YAML::Load(yaml_content);
    SiteConfig config;
    config.load_from_yaml(node);
    
    ASSERT_EQ(config.start_urls.size(), 1);
    EXPECT_EQ(config.start_urls[0], "https://example.com/docs");
    EXPECT_EQ(config.allowed_domain, "example.com");
    EXPECT_EQ(config.allowed_path_prefix, "/docs");
    EXPECT_EQ(config.content_selector, "main");
    EXPECT_EQ(config.max_depth, 3);
    EXPECT_EQ(config.delay_per_host.count(), 500);
    EXPECT_FALSE(config.skip_images);
}

TEST(SiteConfigTest, ValidationMissingStartUrls) {
    SiteConfig config;
    config.allowed_domain = "example.com";
    // start_urls is empty
    
    auto result = config.validate();
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.error.empty());
}

TEST(SiteConfigTest, ValidationMissingDomain) {
    SiteConfig config;
    config.start_urls.push_back("https://example.com");
    // allowed_domain is empty
    
    auto result = config.validate();
    EXPECT_FALSE(result.valid);
}

TEST(SiteConfigTest, ValidationValid) {
    SiteConfig config;
    config.start_urls.push_back("https://example.com/docs");
    config.allowed_domain = "example.com";
    config.delay_per_host = std::chrono::milliseconds(500);
    
    auto result = config.validate();
    EXPECT_TRUE(result.valid);
}

TEST(SiteConfigTest, IsAutoSelector) {
    SiteConfig config;
    config.content_selector = "auto";
    EXPECT_TRUE(config.is_auto_selector());
    
    config.content_selector = "main";
    EXPECT_FALSE(config.is_auto_selector());
}

TEST(SiteConfigTest, DisallowedPathPatterns) {
    SiteConfig config;
    config.disallowed_path_patterns.push_back("/api/.*");
    config.disallowed_path_regex.emplace_back("/api/.*");
    
    EXPECT_FALSE(config.is_path_allowed("/api/users"));
    EXPECT_FALSE(config.is_path_allowed("/api/v2/data"));
    EXPECT_TRUE(config.is_path_allowed("/docs/guide"));
}

TEST(SiteConfigTest, ImageDomainAllowed) {
    SiteConfig config;
    config.allowed_image_domains.push_back("*.example.com");
    config.allowed_image_domains.push_back("cdn.images.io");
    
    EXPECT_TRUE(config.is_image_domain_allowed("cdn.example.com"));
    EXPECT_TRUE(config.is_image_domain_allowed("static.example.com"));
    EXPECT_TRUE(config.is_image_domain_allowed("cdn.images.io"));
    EXPECT_FALSE(config.is_image_domain_allowed("malicious.com"));
}

TEST(SiteConfigTest, ImageDomainAllowedEmptyList) {
    SiteConfig config;
    // No allowed_image_domains = allow all
    EXPECT_TRUE(config.is_image_domain_allowed("any-domain.com"));
}

// ============================================================================
// AppConfig Tests
// ============================================================================

TEST(AppConfigTest, LoadFromString) {
    std::string yaml_content = R"(
default_delay_per_host: 500ms
num_workers: 4
output_base_dir: ./output
sites:
  test_site:
    start_urls:
      - https://example.com
    allowed_domain: example.com
    max_depth: 2
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    
    EXPECT_EQ(config.default_delay_per_host.count(), 500);
    EXPECT_EQ(config.num_workers, 4);
    EXPECT_EQ(config.output_base_dir, "./output");
    EXPECT_TRUE(config.has_site("test_site"));
    EXPECT_FALSE(config.has_site("nonexistent"));
}

TEST(AppConfigTest, GetSiteKeys) {
    std::string yaml_content = R"(
sites:
  site_a:
    start_urls: [https://a.com]
    allowed_domain: a.com
  site_b:
    start_urls: [https://b.com]
    allowed_domain: b.com
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    auto keys = config.get_site_keys();
    
    EXPECT_EQ(keys.size(), 2);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "site_a") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "site_b") != keys.end());
}

TEST(AppConfigTest, GetSiteThrowsForUnknown) {
    std::string yaml_content = R"(
sites:
  test_site:
    start_urls: [https://example.com]
    allowed_domain: example.com
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    EXPECT_THROW(config.get_site("nonexistent"), ConfigError);
}

TEST(AppConfigTest, Validation) {
    std::string yaml_content = R"(
num_workers: 4
sites:
  valid_site:
    start_urls: [https://example.com]
    allowed_domain: example.com
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    auto result = config.validate();
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST(AppConfigTest, ValidationInvalidWorkers) {
    std::string yaml_content = R"(
num_workers: 0
sites:
  test_site:
    start_urls: [https://example.com]
    allowed_domain: example.com
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    auto result = config.validate();
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST(AppConfigTest, FallbackSettings) {
    std::string yaml_content = R"(
enable_output_mapping: true
output_mapping_filename: global_mapping.tsv
sites:
  site_without_override:
    start_urls: [https://example.com]
    allowed_domain: example.com
  site_with_override:
    start_urls: [https://other.com]
    allowed_domain: other.com
    enable_output_mapping: false
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    
    // Site without override uses global
    EXPECT_TRUE(config.get_enable_output_mapping("site_without_override"));
    EXPECT_EQ(config.get_output_mapping_filename("site_without_override"), "global_mapping.tsv");
    
    // Site with override uses its own
    EXPECT_FALSE(config.get_enable_output_mapping("site_with_override"));
}

TEST(AppConfigTest, HTTPClientSettings) {
    std::string yaml_content = R"(
http_client_settings:
  timeout: 60s
  max_idle_conns: 50
  user_agent: TestBot/1.0
sites:
  test_site:
    start_urls: [https://example.com]
    allowed_domain: example.com
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    
    EXPECT_EQ(config.http_client_settings.timeout.count(), 60000);
    EXPECT_EQ(config.http_client_settings.max_idle_conns, 50);
    EXPECT_EQ(config.http_client_settings.user_agent, "TestBot/1.0");
}

TEST(AppConfigTest, LLMGuidanceSettings) {
    std::string yaml_content = R"(
llm_guidance:
  enabled: true
  model: gpt-4
  max_suggestions: 10
  relevance_threshold: 0.8
sites:
  test_site:
    start_urls: [https://example.com]
    allowed_domain: example.com
)";
    
    auto config = AppConfig::load_from_string(yaml_content);
    
    EXPECT_TRUE(config.llm_guidance.enabled);
    EXPECT_EQ(config.llm_guidance.model, "gpt-4");
    EXPECT_EQ(config.llm_guidance.max_suggestions, 10);
    EXPECT_DOUBLE_EQ(config.llm_guidance.relevance_threshold, 0.8);
}

TEST(AppConfigTest, InvalidYaml) {
    std::string invalid_yaml = R"(
this is not valid yaml:
  - missing quotes on key with colon
  invalid: [unclosed bracket
)";
    
    EXPECT_THROW(AppConfig::load_from_string(invalid_yaml), ConfigError);
}
