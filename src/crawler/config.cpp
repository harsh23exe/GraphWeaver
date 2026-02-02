// config.cpp - Configuration Parser Implementation
// LLM Documentation Scraper - C++ Implementation

#include "crawler/config.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace docscraper::crawler {

// ============================================================================
// Duration parsing helpers
// ============================================================================

std::chrono::milliseconds parse_duration(const std::string& str) {
    if (str.empty()) {
        return std::chrono::milliseconds(0);
    }
    
    std::string value;
    std::string unit;
    
    // Split into numeric value and unit
    size_t i = 0;
    while (i < str.length() && (std::isdigit(str[i]) || str[i] == '.')) {
        value += str[i];
        ++i;
    }
    unit = str.substr(i);
    
    // Trim whitespace from unit
    while (!unit.empty() && std::isspace(unit.front())) unit.erase(0, 1);
    while (!unit.empty() && std::isspace(unit.back())) unit.pop_back();
    
    if (value.empty()) {
        throw ConfigError("Invalid duration format: " + str);
    }
    
    double num = std::stod(value);
    
    // Convert to milliseconds based on unit
    if (unit == "ms" || unit == "millisecond" || unit == "milliseconds") {
        return std::chrono::milliseconds(static_cast<long long>(num));
    } else if (unit == "s" || unit == "sec" || unit == "second" || unit == "seconds" || unit.empty()) {
        return std::chrono::milliseconds(static_cast<long long>(num * 1000));
    } else if (unit == "m" || unit == "min" || unit == "minute" || unit == "minutes") {
        return std::chrono::milliseconds(static_cast<long long>(num * 60 * 1000));
    } else if (unit == "h" || unit == "hr" || unit == "hour" || unit == "hours") {
        return std::chrono::milliseconds(static_cast<long long>(num * 60 * 60 * 1000));
    } else {
        throw ConfigError("Unknown duration unit: " + unit);
    }
}

std::string duration_to_string(std::chrono::milliseconds ms) {
    auto count = ms.count();
    if (count < 1000) {
        return std::to_string(count) + "ms";
    } else if (count < 60000) {
        return std::to_string(count / 1000) + "s";
    } else if (count < 3600000) {
        return std::to_string(count / 60000) + "m";
    } else {
        return std::to_string(count / 3600000) + "h";
    }
}

// ============================================================================
// HTTPClientSettings implementation
// ============================================================================

void HTTPClientSettings::load_from_yaml(const YAML::Node& node) {
    if (!node) return;
    
    if (node["timeout"]) {
        timeout = parse_duration(node["timeout"].as<std::string>());
    }
    if (node["max_idle_conns"]) {
        max_idle_conns = node["max_idle_conns"].as<int>();
    }
    if (node["max_idle_conns_per_host"]) {
        max_idle_conns_per_host = node["max_idle_conns_per_host"].as<int>();
    }
    if (node["idle_conn_timeout"]) {
        auto ms = parse_duration(node["idle_conn_timeout"].as<std::string>());
        idle_conn_timeout = std::chrono::duration_cast<std::chrono::seconds>(ms);
    }
    if (node["user_agent"]) {
        user_agent = node["user_agent"].as<std::string>();
    }
    if (node["follow_redirects"]) {
        follow_redirects = node["follow_redirects"].as<bool>();
    }
    if (node["max_redirects"]) {
        max_redirects = node["max_redirects"].as<int>();
    }
}

// ============================================================================
// LLMGuidanceConfig implementation
// ============================================================================

void LLMGuidanceConfig::load_from_yaml(const YAML::Node& node) {
    if (!node) return;
    
    if (node["enabled"]) {
        enabled = node["enabled"].as<bool>();
    }
    if (node["api_endpoint"]) {
        api_endpoint = node["api_endpoint"].as<std::string>();
    }
    if (node["api_key_env"]) {
        api_key_env = node["api_key_env"].as<std::string>();
    }
    if (node["model"]) {
        model = node["model"].as<std::string>();
    }
    if (node["max_suggestions"]) {
        max_suggestions = node["max_suggestions"].as<int>();
    }
    if (node["relevance_threshold"]) {
        relevance_threshold = node["relevance_threshold"].as<double>();
    }
    if (node["crawl_objective"]) {
        crawl_objective = node["crawl_objective"].as<std::string>();
    }
    if (node["timeout"]) {
        timeout = parse_duration(node["timeout"].as<std::string>());
    }
    if (node["max_retries"]) {
        max_retries = node["max_retries"].as<int>();
    }
}

std::string LLMGuidanceConfig::get_api_key() const {
    const char* key = std::getenv(api_key_env.c_str());
    return key ? std::string(key) : "";
}

// ============================================================================
// SiteConfig implementation
// ============================================================================

void SiteConfig::load_from_yaml(const YAML::Node& node) {
    if (!node) return;
    
    // Required fields
    if (node["start_urls"]) {
        for (const auto& url : node["start_urls"]) {
            start_urls.push_back(url.as<std::string>());
        }
    }
    if (node["allowed_domain"]) {
        allowed_domain = node["allowed_domain"].as<std::string>();
    }
    
    // Optional fields
    if (node["allowed_path_prefix"]) {
        allowed_path_prefix = node["allowed_path_prefix"].as<std::string>();
    }
    if (node["content_selector"]) {
        content_selector = node["content_selector"].as<std::string>();
    }
    if (node["max_depth"]) {
        max_depth = node["max_depth"].as<int>();
    }
    if (node["delay_per_host"]) {
        delay_per_host = parse_duration(node["delay_per_host"].as<std::string>());
    }
    
    // Image handling
    if (node["skip_images"]) {
        skip_images = node["skip_images"].as<bool>();
    }
    if (node["max_image_size_bytes"]) {
        max_image_size_bytes = node["max_image_size_bytes"].as<int64_t>();
    }
    if (node["allowed_image_domains"]) {
        for (const auto& domain : node["allowed_image_domains"]) {
            allowed_image_domains.push_back(domain.as<std::string>());
        }
    }
    
    // Path filtering
    if (node["disallowed_path_patterns"]) {
        for (const auto& pattern : node["disallowed_path_patterns"]) {
            std::string pat = pattern.as<std::string>();
            disallowed_path_patterns.push_back(pat);
            try {
                disallowed_path_regex.emplace_back(pat);
            } catch (const std::regex_error& e) {
                spdlog::warn("Invalid regex pattern '{}': {}", pat, e.what());
            }
        }
    }
    
    // Output options
    if (node["enable_output_mapping"]) {
        enable_output_mapping = node["enable_output_mapping"].as<bool>();
    }
    if (node["output_mapping_filename"]) {
        output_mapping_filename = node["output_mapping_filename"].as<std::string>();
    }
    if (node["enable_metadata_yaml"]) {
        enable_metadata_yaml = node["enable_metadata_yaml"].as<bool>();
    }
    if (node["metadata_yaml_filename"]) {
        metadata_yaml_filename = node["metadata_yaml_filename"].as<std::string>();
    }
    if (node["enable_jsonl"]) {
        enable_jsonl = node["enable_jsonl"].as<bool>();
    }
    if (node["enable_structured_json"]) {
        enable_structured_json = node["enable_structured_json"].as<bool>();
    }
    
    // Robots.txt handling
    if (node["respect_robots_txt"]) {
        respect_robots_txt = node["respect_robots_txt"].as<bool>();
    }
    if (node["respect_nofollow"]) {
        respect_nofollow = node["respect_nofollow"].as<bool>();
    }
    
    // LLM guidance
    if (node["llm_guidance"]) {
        llm_guidance = LLMGuidanceConfig();
        llm_guidance->load_from_yaml(node["llm_guidance"]);
    }
}

SiteConfig::ValidationResult SiteConfig::validate() const {
    ValidationResult result;
    
    // Required: start_urls
    if (start_urls.empty()) {
        result.error = "start_urls is required and must not be empty";
        result.valid = false;
        return result;
    }
    
    // Required: allowed_domain
    if (allowed_domain.empty()) {
        result.error = "allowed_domain is required";
        result.valid = false;
        return result;
    }
    
    // Validate start URLs match allowed domain
    for (const auto& url : start_urls) {
        // Simple domain check (could be more sophisticated)
        if (url.find(allowed_domain) == std::string::npos) {
            result.warnings.push_back(
                "start_url '" + url + "' may not match allowed_domain '" + allowed_domain + "'");
        }
    }
    
    // Validate max_depth
    if (max_depth < 0) {
        result.error = "max_depth must be >= 0 (0 means unlimited)";
        result.valid = false;
        return result;
    }
    
    // Validate delay
    if (delay_per_host.count() < 0) {
        result.error = "delay_per_host must be >= 0";
        result.valid = false;
        return result;
    }
    
    // Warn about very short delays
    if (delay_per_host.count() < 100) {
        result.warnings.push_back(
            "delay_per_host < 100ms may be too aggressive. Consider increasing to be polite.");
    }
    
    return result;
}

bool SiteConfig::is_path_allowed(const std::string& path) const {
    for (const auto& regex : disallowed_path_regex) {
        if (std::regex_search(path, regex)) {
            return false;
        }
    }
    return true;
}

bool SiteConfig::is_image_domain_allowed(const std::string& domain) const {
    if (allowed_image_domains.empty()) {
        // If no whitelist, allow all domains
        return true;
    }
    
    for (const auto& allowed : allowed_image_domains) {
        // Support wildcard matching
        if (allowed == "*") return true;
        if (allowed[0] == '*') {
            // *.example.com matches any subdomain
            std::string suffix = allowed.substr(1);
            if (domain.length() >= suffix.length() &&
                domain.compare(domain.length() - suffix.length(), suffix.length(), suffix) == 0) {
                return true;
            }
        } else if (domain == allowed) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// AppConfig implementation
// ============================================================================

AppConfig AppConfig::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw ConfigError("Cannot open config file: " + path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

AppConfig AppConfig::load_from_string(const std::string& yaml_content) {
    AppConfig config;
    
    try {
        YAML::Node root = YAML::Load(yaml_content);
        
        // Global settings
        if (root["default_delay_per_host"]) {
            config.default_delay_per_host = parse_duration(
                root["default_delay_per_host"].as<std::string>());
        }
        if (root["num_workers"]) {
            config.num_workers = root["num_workers"].as<int>();
        }
        if (root["num_image_workers"]) {
            config.num_image_workers = root["num_image_workers"].as<int>();
        }
        if (root["max_requests"]) {
            config.max_requests = root["max_requests"].as<int>();
        }
        if (root["max_requests_per_host"]) {
            config.max_requests_per_host = root["max_requests_per_host"].as<int>();
        }
        if (root["output_base_dir"]) {
            config.output_base_dir = root["output_base_dir"].as<std::string>();
        }
        if (root["state_dir"]) {
            config.state_dir = root["state_dir"].as<std::string>();
        }
        if (root["max_retries"]) {
            config.max_retries = root["max_retries"].as<int>();
        }
        if (root["initial_retry_delay"]) {
            auto ms = parse_duration(root["initial_retry_delay"].as<std::string>());
            config.initial_retry_delay = std::chrono::duration_cast<std::chrono::seconds>(ms);
        }
        if (root["max_retry_delay"]) {
            auto ms = parse_duration(root["max_retry_delay"].as<std::string>());
            config.max_retry_delay = std::chrono::duration_cast<std::chrono::seconds>(ms);
        }
        if (root["semaphore_acquire_timeout"]) {
            auto ms = parse_duration(root["semaphore_acquire_timeout"].as<std::string>());
            config.semaphore_acquire_timeout = std::chrono::duration_cast<std::chrono::seconds>(ms);
        }
        if (root["global_crawl_timeout"]) {
            auto val = root["global_crawl_timeout"];
            if (val.IsScalar() && val.as<std::string>() == "0") {
                config.global_crawl_timeout = std::chrono::seconds(0);
            } else {
                auto ms = parse_duration(val.as<std::string>());
                config.global_crawl_timeout = std::chrono::duration_cast<std::chrono::seconds>(ms);
            }
        }
        
        // Global image settings
        if (root["skip_images"]) {
            config.skip_images = root["skip_images"].as<bool>();
        }
        if (root["max_image_size_bytes"]) {
            config.max_image_size_bytes = root["max_image_size_bytes"].as<int64_t>();
        }
        
        // Output options
        if (root["enable_output_mapping"]) {
            config.enable_output_mapping = root["enable_output_mapping"].as<bool>();
        }
        if (root["output_mapping_filename"]) {
            config.output_mapping_filename = root["output_mapping_filename"].as<std::string>();
        }
        if (root["enable_metadata_yaml"]) {
            config.enable_metadata_yaml = root["enable_metadata_yaml"].as<bool>();
        }
        if (root["metadata_yaml_filename"]) {
            config.metadata_yaml_filename = root["metadata_yaml_filename"].as<std::string>();
        }
        if (root["enable_jsonl"]) {
            config.enable_jsonl = root["enable_jsonl"].as<bool>();
        }
        if (root["enable_structured_json"]) {
            config.enable_structured_json = root["enable_structured_json"].as<bool>();
        }
        if (root["structured_json_schema"]) {
            config.structured_json_schema = root["structured_json_schema"].as<std::string>();
        }
        
        // HTTP client settings
        config.http_client_settings.load_from_yaml(root["http_client_settings"]);
        
        // LLM guidance
        config.llm_guidance.load_from_yaml(root["llm_guidance"]);
        
        // Sites
        if (root["sites"]) {
            for (const auto& site_pair : root["sites"]) {
                std::string site_key = site_pair.first.as<std::string>();
                SiteConfig site_config;
                site_config.load_from_yaml(site_pair.second);
                
                // Apply global defaults if not set at site level
                if (site_config.delay_per_host.count() == 0) {
                    site_config.delay_per_host = config.default_delay_per_host;
                }
                
                config.sites[site_key] = std::move(site_config);
            }
        }
        
    } catch (const YAML::Exception& e) {
        throw ConfigError("YAML parsing error: " + std::string(e.what()));
    }
    
    return config;
}

AppConfig::ValidationResult AppConfig::validate() const {
    ValidationResult result;
    
    // Validate global settings
    if (num_workers < 1) {
        result.errors.push_back("num_workers must be >= 1");
    }
    if (num_image_workers < 0) {
        result.errors.push_back("num_image_workers must be >= 0");
    }
    if (max_retries < 0) {
        result.errors.push_back("max_retries must be >= 0");
    }
    
    // Validate each site
    for (const auto& [key, site] : sites) {
        auto site_result = site.validate();
        if (!site_result.valid) {
            result.errors.push_back("Site '" + key + "': " + site_result.error);
        }
        for (const auto& warning : site_result.warnings) {
            result.warnings.push_back("Site '" + key + "': " + warning);
        }
    }
    
    // Check if any sites are configured
    if (sites.empty()) {
        result.warnings.push_back("No sites configured");
    }
    
    // LLM guidance validation
    if (llm_guidance.enabled && llm_guidance.get_api_key().empty()) {
        result.warnings.push_back(
            "LLM guidance enabled but " + llm_guidance.api_key_env + " environment variable not set");
    }
    
    result.valid = result.errors.empty();
    return result;
}

std::vector<std::string> AppConfig::get_site_keys() const {
    std::vector<std::string> keys;
    keys.reserve(sites.size());
    for (const auto& [key, _] : sites) {
        keys.push_back(key);
    }
    return keys;
}

bool AppConfig::has_site(const std::string& key) const {
    return sites.find(key) != sites.end();
}

const SiteConfig& AppConfig::get_site(const std::string& key) const {
    auto it = sites.find(key);
    if (it == sites.end()) {
        throw ConfigError("Unknown site: " + key);
    }
    return it->second;
}

bool AppConfig::get_skip_images(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.skip_images;
}

bool AppConfig::get_enable_output_mapping(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.enable_output_mapping.value_or(enable_output_mapping);
}

std::string AppConfig::get_output_mapping_filename(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.output_mapping_filename.value_or(output_mapping_filename);
}

bool AppConfig::get_enable_metadata_yaml(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.enable_metadata_yaml.value_or(enable_metadata_yaml);
}

std::string AppConfig::get_metadata_yaml_filename(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.metadata_yaml_filename.value_or(metadata_yaml_filename);
}

bool AppConfig::get_enable_jsonl(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.enable_jsonl.value_or(enable_jsonl);
}

bool AppConfig::get_enable_structured_json(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.enable_structured_json.value_or(enable_structured_json);
}

std::chrono::milliseconds AppConfig::get_delay_per_host(const std::string& site_key) const {
    const auto& site = get_site(site_key);
    return site.delay_per_host;
}

} // namespace docscraper::crawler
