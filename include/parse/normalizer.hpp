// normalizer.hpp - URL Normalization and Scope Validation
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <optional>
#include <vector>

namespace docscraper::parse {

// Parsed URL components
struct URLComponents {
    std::string scheme;      // http, https
    std::string host;        // example.com
    int port = 0;            // 0 = default port
    std::string path;        // /docs/page
    std::string query;       // key=value&other=val
    std::string fragment;    // section-anchor
    
    // Reconstruct URL from components
    std::string to_string(bool include_fragment = false) const;
    
    // Get domain (alias for host)
    std::string domain() const { return host; }
    
    // Get effective port (default to 80/443 based on scheme)
    int effective_port() const;
    
    // Check if URL uses default port for its scheme
    bool is_default_port() const;
};

class URLNormalizer {
public:
    // Parse URL into components
    // Returns nullopt if URL is invalid
    static std::optional<URLComponents> parse(const std::string& url_str);
    
    // Normalize URL:
    // - Lowercase scheme and host
    // - Strip fragment (by default)
    // - Remove default port
    // - Sort query parameters alphabetically
    // - Remove trailing slash (except for root path)
    // - Decode percent-encoded characters where safe
    static std::string normalize(const std::string& url_str, bool keep_fragment = false);
    
    // Check if URL is in scope for crawling
    static bool is_in_scope(
        const std::string& url_str,
        const std::string& allowed_domain,
        const std::string& allowed_path_prefix = ""
    );
    
    // Resolve relative URL against base URL
    // e.g., resolve("https://example.com/docs/", "../images/logo.png") 
    //       -> "https://example.com/images/logo.png"
    static std::optional<std::string> resolve(
        const std::string& base_url,
        const std::string& relative_url
    );
    
    // Extract domain from URL
    static std::string extract_domain(const std::string& url_str);
    
    // Extract path from URL
    static std::string extract_path(const std::string& url_str);
    
    // Check if URL is absolute (has scheme)
    static bool is_absolute(const std::string& url_str);
    
    // Check if URL is a valid HTTP(S) URL
    static bool is_valid_http_url(const std::string& url_str);
    
    // Get file extension from URL path (if any)
    // e.g., "/docs/file.pdf" -> "pdf"
    static std::string get_extension(const std::string& url_str);
    
    // URL encode/decode
    static std::string url_encode(const std::string& str);
    static std::string url_decode(const std::string& str);
    
private:
    // Helper: Lowercase a string
    static std::string to_lower(const std::string& str);
    
    // Helper: Sort query parameters
    static std::string sort_query_params(const std::string& query);
    
    // Helper: Normalize path (remove ., .., double slashes)
    static std::string normalize_path(const std::string& path);
    
    // Helper: Check if character is safe for URL
    static bool is_url_safe(char c);
};

} // namespace docscraper::parse
