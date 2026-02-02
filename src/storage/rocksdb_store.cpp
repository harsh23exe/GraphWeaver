// rocksdb_store.cpp - RocksDB Store Implementation
// LLM Documentation Scraper - C++ Implementation

#include "storage/rocksdb_store.hpp"
#include "utils/errors.hpp"
#include <filesystem>
#include <fstream>

namespace docscraper::storage {

using docscraper::utils::url_hash;

static std::string build_db_path(const std::string& state_dir, const std::string& site_domain) {
    std::filesystem::path path(state_dir);
    path /= site_domain;
    return path.string();
}

RocksDBStore::RocksDBStore(
    const std::string& state_dir,
    const std::string& site_domain,
    bool resume
)
    : db_path_(build_db_path(state_dir, site_domain))
    , site_domain_(site_domain) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.max_open_files = 256;

    if (!resume) {
        // Remove existing DB for fresh crawl
        rocksdb::DestroyDB(db_path_, options);
    }

    // Ensure parent directory exists
    std::filesystem::create_directories(db_path_);

    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path_, &db_ptr);
    if (!status.ok()) {
        throw docscraper::utils::StorageError(
            "Failed to open RocksDB: " + status.ToString());
    }
    db_.reset(db_ptr);
}

RocksDBStore::~RocksDBStore() {
    close();
}

void RocksDBStore::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    db_.reset();
}

std::string RocksDBStore::page_key(const std::string& normalized_url) const {
    return "p:" + url_hash(normalized_url);
}

std::string RocksDBStore::image_key(const std::string& normalized_url) const {
    return "i:" + url_hash(normalized_url);
}

std::optional<std::string> RocksDBStore::get(const std::string& key) {
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
    if (status.IsNotFound()) {
        return std::nullopt;
    }
    if (!status.ok()) {
        throw docscraper::utils::StorageError("RocksDB Get failed: " + status.ToString());
    }
    return value;
}

void RocksDBStore::put(const std::string& key, const std::string& value) {
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok()) {
        throw docscraper::utils::StorageError("RocksDB Put failed: " + status.ToString());
    }
}

void RocksDBStore::del(const std::string& key) {
    rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
    if (!status.ok()) {
        throw docscraper::utils::StorageError("RocksDB Delete failed: " + status.ToString());
    }
}

bool RocksDBStore::mark_page_visited(const std::string& normalized_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = page_key(normalized_url);

    auto existing = get(key);
    if (existing.has_value()) {
        return false;
    }

    models::PageDBEntry entry;
    entry.status = models::PageStatus::Pending;
    entry.normalized_url = normalized_url;
    entry.created_at = std::chrono::system_clock::now();
    entry.last_attempt = entry.created_at;

    put(key, entry.to_json());
    return true;
}

std::pair<models::PageStatus, std::optional<models::PageDBEntry>>
RocksDBStore::check_page_status(const std::string& normalized_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = page_key(normalized_url);

    auto value = get(key);
    if (!value.has_value()) {
        return {models::PageStatus::Unknown, std::nullopt};
    }

    auto entry = models::PageDBEntry::from_json(*value);
    return {entry.status, entry};
}

void RocksDBStore::update_page_status(
    const std::string& normalized_url,
    const models::PageDBEntry& entry
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = page_key(normalized_url);
    put(key, entry.to_json());
}

std::optional<std::string>
RocksDBStore::get_page_content_hash(const std::string& normalized_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = page_key(normalized_url);

    auto value = get(key);
    if (!value.has_value()) {
        return std::nullopt;
    }

    auto entry = models::PageDBEntry::from_json(*value);
    if (entry.content_hash.empty()) {
        return std::nullopt;
    }
    return entry.content_hash;
}

std::pair<models::ImageStatus, std::optional<models::ImageDBEntry>>
RocksDBStore::check_image_status(const std::string& normalized_url) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = image_key(normalized_url);

    auto value = get(key);
    if (!value.has_value()) {
        return {models::ImageStatus::Unknown, std::nullopt};
    }

    auto entry = models::ImageDBEntry::from_json(*value);
    return {entry.status, entry};
}

void RocksDBStore::update_image_status(
    const std::string& normalized_url,
    const models::ImageDBEntry& entry
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = image_key(normalized_url);
    put(key, entry.to_json());
}

int RocksDBStore::get_visited_count() {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    for (it->Seek("p:"); it->Valid(); it->Next()) {
        if (!it->key().starts_with("p:")) {
            break;
        }
        ++count;
    }
    return count;
}

int RocksDBStore::requeue_incomplete(
    std::function<void(models::WorkItem)> enqueue_callback
) {
    std::lock_guard<std::mutex> lock(mutex_);
    int requeued = 0;

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    for (it->Seek("p:"); it->Valid(); it->Next()) {
        if (!it->key().starts_with("p:")) {
            break;
        }

        std::string value = it->value().ToString();
        auto entry = models::PageDBEntry::from_json(value);

        if (entry.status == models::PageStatus::Pending ||
            entry.status == models::PageStatus::Failure) {
            if (!entry.normalized_url.empty()) {
                models::WorkItem item(entry.normalized_url, entry.depth);
                enqueue_callback(std::move(item));
                ++requeued;
            }
        }
    }

    return requeued;
}

void RocksDBStore::write_visited_log(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(file_path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        throw docscraper::utils::IOError("Failed to open visited log: " + file_path);
    }

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        out << it->key().ToString() << "\t" << it->value().ToString() << "\n";
    }
}

} // namespace docscraper::storage
