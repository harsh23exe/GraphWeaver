// config.hpp - Configuration Structures and Parser
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>
#include <regex>
#include <yaml-cpp/yaml.h>

namespace docscraper::crawler {

using namespace std::chrono_literals;

// Duration parsing helper
std::chrono::milliseconds parse_duration(const std::string& str);
std::string duration_to_string(std::chrono::milliseconds ms);

// HTTP client settings
struct HTTPClientSettings {
    std::chrono::milliseconds timeout = 30000ms;
    int max_idle_conns = 100;
    int max_idle_conns_per_host = 10;
    std::chrono::seconds idle_conn_timeout = 90s;
    std::string user_agent = "DocScraper/1.0 (+https://github.com/doc-scraper)";
    bool follow_redirects = true;
    int max_redirects = 10;
    
    void load_from_yaml(const YAML::Node& node);
};

// LLM guidance settings (for AI-guided crawling)
struct LLMGuidanceConfig {
    bool enabled = false;
    std::string api_endpoint = "https://api.openai.com/v1/chat/completions";
    std::string api_key_env = "OPENAI_API_KEY";
    std::string model = "gpt-4o-mini";
    int max_suggestions = 5;
    double relevance_threshold = 0.7;
    std::string crawl_objective;
    std::chrono::milliseconds timeout = 30000ms;
    int max_retries = 3;
    
    void load_from_yaml(const YAML::Node& node);
    std::string get_api_key() const;  // Reads from environment
};

// Per-site configuration
struct SiteConfig {
    std::vector<std::string> start_urls;
    std::string allowed_domain;
    std::string allowed_path_prefix;
    std::string content_selector = "auto";
    int max_depth = 0;  // 0 = unlimited
    std::chrono::milliseconds delay_per_host = 500ms;
    
    // Image handling
    bool skip_images = false;
    int64_t max_image_size_bytes = 10 * 1024 * 1024;  // 10MB
    std::vector<std::string> allowed_image_domains;
    
    // Path filtering
    std::vector<std::string> disallowed_path_patterns;
    std::vector<std::regex> disallowed_path_regex;  // Compiled patterns
    
    // Output options (can override global)
    std::optional<bool> enable_output_mapping;
    std::optional<std::string> output_mapping_filename;
    std::optional<bool> enable_metadata_yaml;
    std::optional<std::string> metadata_yaml_filename;
    std::optional<bool> enable_jsonl;
    std::optional<bool> enable_structured_json;
    
    // Robots.txt handling
    bool respect_robots_txt = true;
    bool respect_nofollow = true;
    
    // LLM guidance (can override global)
    std::optional<LLMGuidanceConfig> llm_guidance;
    
    // Load from YAML
    void load_from_yaml(const YAML::Node& node);
    
    // Validation
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> warnings;
        std::string error;
    };
    ValidationResult validate() const;
    
    // Helper methods
    bool is_path_allowed(const std::string& path) const;
    bool is_image_domain_allowed(const std::string& domain) const;
    bool is_auto_selector() const { return content_selector == "auto"; }
};

// Global application configuration
struct AppConfig {
    // Global defaults
    std::chrono::milliseconds default_delay_per_host = 500ms;
    int num_workers = 8;
    int num_image_workers = 4;
    int max_requests = 100;
    int max_requests_per_host = 10;
    
    // Directories
    std::string output_base_dir = "./crawled_docs";
    std::string state_dir = "./crawler_state";
    
    // Retry settings
    int max_retries = 3;
    std::chrono::seconds initial_retry_delay = 1s;
    std::chrono::seconds max_retry_delay = 30s;
    
    // Timeouts
    std::chrono::seconds semaphore_acquire_timeout = 60s;
    std::chrono::seconds global_crawl_timeout = 0s;  // 0 = unlimited
    
    // Global image settings (can be overridden per-site)
    bool skip_images = false;
    int64_t max_image_size_bytes = 10 * 1024 * 1024;
    
    // Global output options
    bool enable_output_mapping = true;
    std::string output_mapping_filename = "url_mapping.tsv";
    bool enable_metadata_yaml = true;
    std::string metadata_yaml_filename = "crawl_metadata.yaml";
    bool enable_jsonl = true;
    bool enable_structured_json = false;
    std::string structured_json_schema;
    
    // HTTP client
    HTTPClientSettings http_client_settings;
    
    // LLM guidance (global settings)
    LLMGuidanceConfig llm_guidance;
    
    // Sites
    std::map<std::string, SiteConfig> sites;
    
    // Load from file
    static AppConfig load_from_file(const std::string& path);
    static AppConfig load_from_string(const std::string& yaml_content);
    
    // Validation
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };
    ValidationResult validate() const;
    
    // Helper methods
    std::vector<std::string> get_site_keys() const;
    bool has_site(const std::string& key) const;
    const SiteConfig& get_site(const std::string& key) const;
    
    // Get effective settings for a site (with fallbacks to global)
    bool get_skip_images(const std::string& site_key) const;
    bool get_enable_output_mapping(const std::string& site_key) const;
    std::string get_output_mapping_filename(const std::string& site_key) const;
    bool get_enable_metadata_yaml(const std::string& site_key) const;
    std::string get_metadata_yaml_filename(const std::string& site_key) const;
    bool get_enable_jsonl(const std::string& site_key) const;
    bool get_enable_structured_json(const std::string& site_key) const;
    std::chrono::milliseconds get_delay_per_host(const std::string& site_key) const;
};

// Exception for config errors
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace docscraper::crawler
