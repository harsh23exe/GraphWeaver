// hash.cpp - Cryptographic Hashing Implementation
// LLM Documentation Scraper - C++ Implementation

#include "utils/hash.hpp"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace docscraper::utils {

std::string bytes_to_hex(const unsigned char* data, size_t length) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string bytes_to_hex(const std::vector<unsigned char>& data) {
    return bytes_to_hex(data.data(), data.size());
}

std::vector<unsigned char> hex_to_bytes(const std::string& hex) {
    std::vector<unsigned char> result;
    result.reserve(hex.length() / 2);
    
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        unsigned int byte;
        std::istringstream iss(hex.substr(i, 2));
        iss >> std::hex >> byte;
        result.push_back(static_cast<unsigned char>(byte));
    }
    
    return result;
}

std::string md5_hash(const void* data, size_t length) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create MD5 context");
    }
    
    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    EVP_DigestUpdate(ctx, data, length);
    EVP_DigestFinal_ex(ctx, digest, nullptr);
    EVP_MD_CTX_free(ctx);
    
    return bytes_to_hex(digest, MD5_DIGEST_LENGTH);
}

std::string md5_hash(const std::string& data) {
    return md5_hash(data.data(), data.size());
}

std::string sha256_hash(const void* data, size_t length) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create SHA256 context");
    }
    
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data, length);
    EVP_DigestFinal_ex(ctx, digest, nullptr);
    EVP_MD_CTX_free(ctx);
    
    return bytes_to_hex(digest, SHA256_DIGEST_LENGTH);
}

std::string sha256_hash(const std::string& data) {
    return sha256_hash(data.data(), data.size());
}

std::string sha1_hash(const std::string& data) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create SHA1 context");
    }
    
    EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, digest, nullptr);
    EVP_MD_CTX_free(ctx);
    
    return bytes_to_hex(digest, SHA_DIGEST_LENGTH);
}

std::string content_hash(const std::string& content) {
    // Use SHA256 for content hashing (good collision resistance)
    return sha256_hash(content);
}

std::string url_hash(const std::string& normalized_url) {
    // Use MD5 for URL hashing (shorter, suitable for keys)
    return md5_hash(normalized_url);
}

} // namespace docscraper::utils
