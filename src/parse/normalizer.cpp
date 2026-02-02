// normalizer.cpp - URL Normalization Implementation
// LLM Documentation Scraper - C++ Implementation

#include "parse/normalizer.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <regex>
#include <map>
#include <iomanip>

namespace docscraper::parse {

// ============================================================================
// URLComponents implementation
// ============================================================================

std::string URLComponents::to_string(bool include_fragment) const {
    std::ostringstream oss;
    
    // Scheme
    oss << scheme << "://";
    
    // Host
    oss << host;
    
    // Port (only if non-default)
    if (port > 0 && !is_default_port()) {
        oss << ":" << port;
    }
    
    // Path (at minimum, should be "/")
    if (path.empty()) {
        oss << "/";
    } else {
        oss << path;
    }
    
    // Query
    if (!query.empty()) {
        oss << "?" << query;
    }
    
    // Fragment
    if (include_fragment && !fragment.empty()) {
        oss << "#" << fragment;
    }
    
    return oss.str();
}

int URLComponents::effective_port() const {
    if (port > 0) return port;
    if (scheme == "https") return 443;
    if (scheme == "http") return 80;
    return 0;
}

bool URLComponents::is_default_port() const {
    if (port == 0) return true;
    if (scheme == "https" && port == 443) return true;
    if (scheme == "http" && port == 80) return true;
    return false;
}

// ============================================================================
// URLNormalizer implementation
// ============================================================================

std::string URLNormalizer::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool URLNormalizer::is_url_safe(char c) {
    // Unreserved characters (RFC 3986)
    if (std::isalnum(static_cast<unsigned char>(c))) return true;
    if (c == '-' || c == '_' || c == '.' || c == '~') return true;
    return false;
}

std::string URLNormalizer::url_encode(const std::string& str) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    
    for (unsigned char c : str) {
        if (is_url_safe(c)) {
            oss << c;
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    
    return oss.str();
}

std::string URLNormalizer::url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value = 0;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}

std::optional<URLComponents> URLNormalizer::parse(const std::string& url_str) {
    if (url_str.empty()) {
        return std::nullopt;
    }
    
    URLComponents result;
    
    // Simple regex-based URL parsing
    // Pattern: scheme://host(:port)?(/path)?(\?query)?(#fragment)?
    static const std::regex url_regex(
        R"(^(https?)://([^/:]+)(?::(\d+))?(/[^?#]*)?(?:\?([^#]*))?(?:#(.*))?$)",
        std::regex::icase
    );
    
    std::smatch match;
    if (!std::regex_match(url_str, match, url_regex)) {
        return std::nullopt;
    }
    
    result.scheme = to_lower(match[1].str());
    result.host = to_lower(match[2].str());
    
    if (match[3].matched && !match[3].str().empty()) {
        result.port = std::stoi(match[3].str());
    }
    
    result.path = match[4].matched ? match[4].str() : "/";
    if (result.path.empty()) {
        result.path = "/";
    }
    
    result.query = match[5].matched ? match[5].str() : "";
    result.fragment = match[6].matched ? match[6].str() : "";
    
    return result;
}

std::string URLNormalizer::sort_query_params(const std::string& query) {
    if (query.empty()) return "";
    
    // Parse query parameters
    std::map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        auto eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            params[key] = value;
        } else if (!pair.empty()) {
            params[pair] = "";
        }
    }
    
    // Rebuild sorted
    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) oss << '&';
        oss << key;
        if (!value.empty()) {
            oss << '=' << value;
        }
        first = false;
    }
    
    return oss.str();
}

std::string URLNormalizer::normalize_path(const std::string& path) {
    if (path.empty()) return "/";
    
    std::vector<std::string> segments;
    std::istringstream iss(path);
    std::string segment;
    
    while (std::getline(iss, segment, '/')) {
        if (segment.empty() || segment == ".") {
            continue;
        } else if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else {
            segments.push_back(segment);
        }
    }
    
    std::ostringstream oss;
    for (const auto& s : segments) {
        oss << "/" << s;
    }
    
    std::string result = oss.str();
    if (result.empty()) {
        result = "/";
    }
    
    // Preserve trailing slash if original had one (and path isn't just "/")
    if (path.length() > 1 && path.back() == '/' && result.back() != '/') {
        result += '/';
    }
    
    return result;
}

std::string URLNormalizer::normalize(const std::string& url_str, bool keep_fragment) {
    auto components = parse(url_str);
    if (!components) {
        return url_str;  // Return as-is if unparseable
    }
    
    // Normalize components
    components->scheme = to_lower(components->scheme);
    components->host = to_lower(components->host);
    components->path = normalize_path(components->path);
    components->query = sort_query_params(components->query);
    
    // Clear fragment unless keeping
    if (!keep_fragment) {
        components->fragment.clear();
    }
    
    // Remove default port
    if (components->is_default_port()) {
        components->port = 0;
    }
    
    // Remove trailing slash for non-directory paths
    // (Keep for paths ending in / that aren't just /)
    if (components->path.length() > 1 && components->path.back() == '/') {
        // Check if it looks like a directory (no file extension)
        auto last_slash = components->path.rfind('/');
        auto after_slash = components->path.substr(last_slash + 1);
        if (after_slash.empty()) {
            // It's definitely a directory path, remove trailing slash
            // Actually, keep it - directories should have trailing slash
        }
    }
    
    return components->to_string(keep_fragment);
}

bool URLNormalizer::is_in_scope(
    const std::string& url_str,
    const std::string& allowed_domain,
    const std::string& allowed_path_prefix
) {
    auto components = parse(url_str);
    if (!components) {
        return false;
    }
    
    // Check scheme (only HTTP/HTTPS allowed)
    if (components->scheme != "http" && components->scheme != "https") {
        return false;
    }
    
    // Check domain
    std::string url_domain = to_lower(components->host);
    std::string check_domain = to_lower(allowed_domain);
    
    // Exact match or subdomain match
    if (url_domain != check_domain) {
        // Check if it's a subdomain
        if (url_domain.length() > check_domain.length()) {
            std::string suffix = "." + check_domain;
            if (url_domain.rfind(suffix) != url_domain.length() - suffix.length()) {
                return false;
            }
        } else {
            return false;
        }
    }
    
    // Check path prefix (if specified)
    if (!allowed_path_prefix.empty()) {
        if (components->path.find(allowed_path_prefix) != 0) {
            return false;
        }
    }
    
    return true;
}

std::optional<std::string> URLNormalizer::resolve(
    const std::string& base_url,
    const std::string& relative_url
) {
    if (relative_url.empty()) {
        return base_url;
    }
    
    // If relative URL is actually absolute, return it
    if (is_absolute(relative_url)) {
        return normalize(relative_url);
    }
    
    auto base = parse(base_url);
    if (!base) {
        return std::nullopt;
    }
    
    // Handle different relative URL formats
    std::string resolved_path;
    std::string resolved_query;
    
    if (relative_url[0] == '/') {
        // Absolute path (relative to root)
        if (relative_url.length() > 1 && relative_url[1] == '/') {
            // Protocol-relative URL: //example.com/path
            std::string full_url = base->scheme + ":" + relative_url;
            auto parsed = parse(full_url);
            if (parsed) {
                return parsed->to_string();
            }
            return std::nullopt;
        }
        
        // Extract path and query from relative URL
        auto query_pos = relative_url.find('?');
        if (query_pos != std::string::npos) {
            resolved_path = relative_url.substr(0, query_pos);
            resolved_query = relative_url.substr(query_pos + 1);
        } else {
            resolved_path = relative_url;
        }
    } else if (relative_url[0] == '?') {
        // Query-only relative URL
        resolved_path = base->path;
        resolved_query = relative_url.substr(1);
    } else if (relative_url[0] == '#') {
        // Fragment-only - return base with fragment
        URLComponents result = *base;
        result.fragment = relative_url.substr(1);
        return result.to_string(true);
    } else {
        // Relative path
        std::string base_dir = base->path;
        auto last_slash = base_dir.rfind('/');
        if (last_slash != std::string::npos) {
            base_dir = base_dir.substr(0, last_slash + 1);
        } else {
            base_dir = "/";
        }
        
        auto query_pos = relative_url.find('?');
        if (query_pos != std::string::npos) {
            resolved_path = base_dir + relative_url.substr(0, query_pos);
            resolved_query = relative_url.substr(query_pos + 1);
        } else {
            resolved_path = base_dir + relative_url;
        }
    }
    
    // Normalize the path
    resolved_path = normalize_path(resolved_path);
    
    // Construct result
    URLComponents result;
    result.scheme = base->scheme;
    result.host = base->host;
    result.port = base->port;
    result.path = resolved_path;
    result.query = resolved_query;
    
    return normalize(result.to_string());
}

std::string URLNormalizer::extract_domain(const std::string& url_str) {
    auto components = parse(url_str);
    return components ? components->host : "";
}

std::string URLNormalizer::extract_path(const std::string& url_str) {
    auto components = parse(url_str);
    return components ? components->path : "";
}

bool URLNormalizer::is_absolute(const std::string& url_str) {
    // Only scheme:// is considered absolute, not protocol-relative //
    return url_str.find("://") != std::string::npos;
}

bool URLNormalizer::is_valid_http_url(const std::string& url_str) {
    auto components = parse(url_str);
    if (!components) return false;
    return components->scheme == "http" || components->scheme == "https";
}

std::string URLNormalizer::get_extension(const std::string& url_str) {
    std::string path = extract_path(url_str);
    
    // Remove query string from path
    auto query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path = path.substr(0, query_pos);
    }
    
    auto dot_pos = path.rfind('.');
    auto slash_pos = path.rfind('/');
    
    // Dot must be after last slash and not at the end
    if (dot_pos != std::string::npos && 
        (slash_pos == std::string::npos || dot_pos > slash_pos) &&
        dot_pos < path.length() - 1) {
        return to_lower(path.substr(dot_pos + 1));
    }
    
    return "";
}

// ============================================================================
// url_to_filepath implementation
// ============================================================================

std::string url_to_filepath(
    const std::string& url,
    const std::string& base_domain,
    const std::string& base_dir
) {
    auto components = URLNormalizer::parse(url);
    if (!components) {
        return "";
    }
    
    std::string path = components->path;
    
    // Remove leading slash
    if (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }
    
    // Replace special characters
    std::string safe_path;
    for (char c : path) {
        if (c == '/' || std::isalnum(static_cast<unsigned char>(c)) || 
            c == '-' || c == '_' || c == '.') {
            safe_path += c;
        } else {
            safe_path += '_';
        }
    }
    
    // Add .md extension if not present
    if (safe_path.empty()) {
        safe_path = "index";
    }
    
    // Remove trailing slash, add .md
    if (safe_path.back() == '/') {
        safe_path = safe_path.substr(0, safe_path.length() - 1);
    }
    
    // Check for extensions
    auto last_dot = safe_path.rfind('.');
    auto last_slash = safe_path.rfind('/');
    
    // Make sure the dot is after the last slash (in the filename, not directory)
    bool has_extension = (last_dot != std::string::npos && 
                          (last_slash == std::string::npos || last_dot > last_slash));
    
    if (has_extension) {
        std::string ext = safe_path.substr(last_dot + 1);
        // Convert to lowercase for comparison
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });
        
        if (ext == "html" || ext == "htm") {
            // Replace .html/.htm with .md
            safe_path = safe_path.substr(0, last_dot) + ".md";
        }
        // Keep other extensions as-is
    } else {
        // No extension, add .md
        safe_path += ".md";
    }
    
    // Construct full path
    std::string result;
    if (!base_dir.empty()) {
        result = base_dir;
        if (result.back() != '/') {
            result += '/';
        }
    }
    result += base_domain + "/" + safe_path;
    
    return result;
}

} // namespace docscraper::parse
