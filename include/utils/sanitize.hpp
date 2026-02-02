// sanitize.hpp - Filename and Path Sanitization
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <vector>

namespace docscraper::utils {

// Sanitize a string for use as a filename
// - Replaces invalid characters with underscores
// - Handles reserved filenames (Windows: CON, NUL, etc.)
// - Truncates to max_length
std::string sanitize_filename(
    const std::string& input,
    size_t max_length = 255
);

// Sanitize a full path (each component separately)
std::string sanitize_path(const std::string& input);

// Replace multiple consecutive characters with a single one
// e.g., "a___b" -> "a_b" when replacing '_'
std::string collapse_duplicates(const std::string& input, char c);

// Truncate string to max bytes, respecting UTF-8 boundaries
std::string truncate_utf8(const std::string& input, size_t max_bytes);

// Remove HTML tags from text
std::string strip_html_tags(const std::string& html);

// Normalize whitespace (collapse multiple spaces, trim)
std::string normalize_whitespace(const std::string& input);

// Escape special characters for TSV output
std::string escape_tsv(const std::string& input);

// Escape special characters for YAML output
std::string escape_yaml(const std::string& input);

// Check if string is valid UTF-8
bool is_valid_utf8(const std::string& input);

// Replace invalid UTF-8 sequences with replacement character
std::string fix_utf8(const std::string& input);

// Generate a safe slug from text (for URLs/filenames)
// e.g., "Hello World!" -> "hello-world"
std::string slugify(const std::string& input);

// Split path into directory and filename
std::pair<std::string, std::string> split_path(const std::string& path);

// Get parent directory
std::string parent_directory(const std::string& path);

// Join path components
std::string join_path(const std::string& a, const std::string& b);
std::string join_path(const std::vector<std::string>& components);

// Ensure directory exists (create if needed)
bool ensure_directory(const std::string& path);

// Get relative path from base to target
std::string relative_path(const std::string& from, const std::string& to);

} // namespace docscraper::utils
