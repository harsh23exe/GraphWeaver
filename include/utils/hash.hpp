// hash.hpp - Cryptographic Hashing Utilities
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <vector>

namespace docscraper::utils {

// Compute MD5 hash of data (returns hex string)
std::string md5_hash(const std::string& data);
std::string md5_hash(const void* data, size_t length);

// Compute SHA256 hash of data (returns hex string)
std::string sha256_hash(const std::string& data);
std::string sha256_hash(const void* data, size_t length);

// Compute SHA1 hash of data (returns hex string)
std::string sha1_hash(const std::string& data);

// Quick hash for content comparison (uses SHA256)
std::string content_hash(const std::string& content);

// Hash URL for use as database key (uses MD5 for shorter keys)
std::string url_hash(const std::string& normalized_url);

// Convert binary hash to hex string
std::string bytes_to_hex(const unsigned char* data, size_t length);
std::string bytes_to_hex(const std::vector<unsigned char>& data);

// Convert hex string to binary
std::vector<unsigned char> hex_to_bytes(const std::string& hex);

} // namespace docscraper::utils
