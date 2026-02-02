// rocksdb_store.hpp - RocksDB Implementation of VisitedStore
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "storage/store_interface.hpp"
#include "utils/hash.hpp"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <memory>
#include <mutex>
#include <string>

namespace docscraper::storage {

class RocksDBStore : public VisitedStore {
public:
    RocksDBStore(
        const std::string& state_dir,
        const std::string& site_domain,
        bool resume
    );
    ~RocksDBStore() override;

    // PageStore interface
    bool mark_page_visited(const std::string& normalized_url) override;
    std::pair<models::PageStatus, std::optional<models::PageDBEntry>>
        check_page_status(const std::string& normalized_url) override;
    void update_page_status(
        const std::string& normalized_url,
        const models::PageDBEntry& entry
    ) override;
    std::optional<std::string>
        get_page_content_hash(const std::string& normalized_url) override;

    // ImageStore interface
    std::pair<models::ImageStatus, std::optional<models::ImageDBEntry>>
        check_image_status(const std::string& normalized_url) override;
    void update_image_status(
        const std::string& normalized_url,
        const models::ImageDBEntry& entry
    ) override;

    // StoreAdmin interface
    int get_visited_count() override;
    int requeue_incomplete(
        std::function<void(models::WorkItem)> enqueue_callback
    ) override;
    void write_visited_log(const std::string& file_path) override;
    void close() override;

private:
    std::unique_ptr<rocksdb::DB> db_;
    std::string db_path_;
    std::string site_domain_;
    std::mutex mutex_;

    std::string page_key(const std::string& normalized_url) const;
    std::string image_key(const std::string& normalized_url) const;

    std::optional<std::string> get(const std::string& key);
    void put(const std::string& key, const std::string& value);
    void del(const std::string& key);
};

} // namespace docscraper::storage
