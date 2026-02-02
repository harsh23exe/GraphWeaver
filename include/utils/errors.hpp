// errors.hpp - Error Types and Exception Classes
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <stdexcept>
#include <string>

namespace docscraper::utils {

// Base exception for all crawler errors
class CrawlerError : public std::runtime_error {
public:
    explicit CrawlerError(const std::string& msg) : std::runtime_error(msg) {}
    virtual ~CrawlerError() = default;
};

// URL is outside allowed scope (domain/path)
class ScopeViolationError : public CrawlerError {
public:
    explicit ScopeViolationError(const std::string& url, const std::string& reason = "")
        : CrawlerError("URL out of scope: " + url + (reason.empty() ? "" : " (" + reason + ")"))
        , url_(url)
        , reason_(reason)
    {}
    
    const std::string& url() const { return url_; }
    const std::string& reason() const { return reason_; }
    
private:
    std::string url_;
    std::string reason_;
};

// Content selector not found in page
class ContentSelectorError : public CrawlerError {
public:
    explicit ContentSelectorError(const std::string& selector, const std::string& url = "")
        : CrawlerError("Content selector not found: " + selector + 
                      (url.empty() ? "" : " in " + url))
        , selector_(selector)
        , url_(url)
    {}
    
    const std::string& selector() const { return selector_; }
    const std::string& url() const { return url_; }
    
private:
    std::string selector_;
    std::string url_;
};

// HTTP request failed
class HTTPError : public CrawlerError {
public:
    HTTPError(int status_code, const std::string& msg)
        : CrawlerError("HTTP error " + std::to_string(status_code) + ": " + msg)
        , status_code_(status_code)
    {}
    
    int status_code() const { return status_code_; }
    
private:
    int status_code_;
};

// Network connection error
class NetworkError : public CrawlerError {
public:
    explicit NetworkError(const std::string& msg)
        : CrawlerError("Network error: " + msg)
    {}
};

// Request timeout
class TimeoutError : public CrawlerError {
public:
    explicit TimeoutError(const std::string& msg)
        : CrawlerError("Timeout: " + msg)
    {}
};

// Rate limit exceeded
class RateLimitError : public CrawlerError {
public:
    explicit RateLimitError(const std::string& msg = "Rate limit exceeded")
        : CrawlerError(msg)
    {}
};

// Robots.txt disallowed
class RobotsDisallowedError : public CrawlerError {
public:
    explicit RobotsDisallowedError(const std::string& url)
        : CrawlerError("Robots.txt disallowed: " + url)
        , url_(url)
    {}
    
    const std::string& url() const { return url_; }
    
private:
    std::string url_;
};

// HTML parsing error
class ParseError : public CrawlerError {
public:
    explicit ParseError(const std::string& msg)
        : CrawlerError("Parse error: " + msg)
    {}
};

// File I/O error
class IOError : public CrawlerError {
public:
    explicit IOError(const std::string& msg)
        : CrawlerError("I/O error: " + msg)
    {}
};

// Storage/database error
class StorageError : public CrawlerError {
public:
    explicit StorageError(const std::string& msg)
        : CrawlerError("Storage error: " + msg)
    {}
};

// Maximum retries exceeded
class MaxRetriesError : public CrawlerError {
public:
    explicit MaxRetriesError(const std::string& url, int attempts)
        : CrawlerError("Max retries exceeded for " + url + " after " + 
                      std::to_string(attempts) + " attempts")
        , url_(url)
        , attempts_(attempts)
    {}
    
    const std::string& url() const { return url_; }
    int attempts() const { return attempts_; }
    
private:
    std::string url_;
    int attempts_;
};

// Content extraction resulted in empty content
class EmptyContentError : public CrawlerError {
public:
    explicit EmptyContentError(const std::string& url)
        : CrawlerError("Empty content extracted from: " + url)
        , url_(url)
    {}
    
    const std::string& url() const { return url_; }
    
private:
    std::string url_;
};

} // namespace docscraper::utils
