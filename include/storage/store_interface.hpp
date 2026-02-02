// store_interface.hpp - Storage Interface for Crawl State
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "models/models.hpp"
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace docscraper::storage {

class PageStore {
public:
    virtual ~PageStore() = default;
    // Returns true if newly added, false if already exists
    virtual bool mark_page_visited(const std::string& normalized_url) = 0;
    virtual std::pair<models::PageStatus, std::optional<models::PageDBEntry>>
        check_page_status(const std::string& normalized_url) = 0;
    virtual void update_page_status(
        const std::string& normalized_url,
        const models::PageDBEntry& entry
    ) = 0;
    virtual std::optional<std::string>
        get_page_content_hash(const std::string& normalized_url) = 0;
};

class ImageStore {
public:
    virtual ~ImageStore() = default;
    virtual std::pair<models::ImageStatus, std::optional<models::ImageDBEntry>>
        check_image_status(const std::string& normalized_url) = 0;
    virtual void update_image_status(
        const std::string& normalized_url,
        const models::ImageDBEntry& entry
    ) = 0;
};

class StoreAdmin {
public:
    virtual ~StoreAdmin() = default;
    virtual int get_visited_count() = 0;
    // For resume functionality: requeue pending/failed pages
    virtual int requeue_incomplete(
        std::function<void(models::WorkItem)> enqueue_callback
    ) = 0;
    virtual void write_visited_log(const std::string& file_path) = 0;
    virtual void close() = 0;
};

class VisitedStore : public PageStore, public ImageStore, public StoreAdmin {
public:
    virtual ~VisitedStore() = default;
};

} // namespace docscraper::storage
