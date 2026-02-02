// test_rocksdb_store.cpp - Unit tests for RocksDB store
#include <gtest/gtest.h>
#include "storage/rocksdb_store.hpp"
#include <filesystem>

using namespace docscraper::storage;
using namespace docscraper::models;

class RocksDBStoreTest : public ::testing::Test {
protected:
    std::string temp_dir = "./test_db";
    std::unique_ptr<RocksDBStore> store;

    void SetUp() override {
        std::filesystem::remove_all(temp_dir);
        store = std::make_unique<RocksDBStore>(temp_dir, "example.com", false);
    }

    void TearDown() override {
        store.reset();
        std::filesystem::remove_all(temp_dir);
    }
};

TEST_F(RocksDBStoreTest, MarkPageVisited) {
    bool is_new = store->mark_page_visited("https://example.com/page1");
    EXPECT_TRUE(is_new);

    is_new = store->mark_page_visited("https://example.com/page1");
    EXPECT_FALSE(is_new);
}

TEST_F(RocksDBStoreTest, PageStatusLifecycle) {
    store->mark_page_visited("https://example.com/page1");

    auto [status, entry] = store->check_page_status("https://example.com/page1");
    EXPECT_EQ(status, PageStatus::Pending);
    ASSERT_TRUE(entry.has_value());

    PageDBEntry updated = *entry;
    updated.mark_in_progress();
    store->update_page_status("https://example.com/page1", updated);

    auto [status2, entry2] = store->check_page_status("https://example.com/page1");
    EXPECT_EQ(status2, PageStatus::InProgress);
}

TEST_F(RocksDBStoreTest, PageContentHash) {
    store->mark_page_visited("https://example.com/page1");
    auto [status, entry] = store->check_page_status("https://example.com/page1");
    ASSERT_TRUE(entry.has_value());

    PageDBEntry updated = *entry;
    updated.content_hash = "hash123";
    store->update_page_status("https://example.com/page1", updated);

    auto hash = store->get_page_content_hash("https://example.com/page1");
    ASSERT_TRUE(hash.has_value());
    EXPECT_EQ(*hash, "hash123");
}

TEST_F(RocksDBStoreTest, ImageStatusLifecycle) {
    ImageDBEntry img;
    img.status = ImageStatus::Pending;
    img.original_url = "https://example.com/img.png";

    store->update_image_status("https://example.com/img.png", img);

    auto [status, entry] = store->check_image_status("https://example.com/img.png");
    EXPECT_EQ(status, ImageStatus::Pending);
    ASSERT_TRUE(entry.has_value());
}

TEST_F(RocksDBStoreTest, GetVisitedCount) {
    store->mark_page_visited("https://example.com/page1");
    store->mark_page_visited("https://example.com/page2");
    store->mark_page_visited("https://example.com/page3");

    int count = store->get_visited_count();
    EXPECT_EQ(count, 3);
}

TEST_F(RocksDBStoreTest, RequeueIncomplete) {
    store->mark_page_visited("https://example.com/page1");
    store->mark_page_visited("https://example.com/page2");

    // Mark page2 as success so only page1 remains pending
    auto [status2, entry2] = store->check_page_status("https://example.com/page2");
    ASSERT_TRUE(entry2.has_value());
    PageDBEntry updated = *entry2;
    updated.mark_success("hash", "/path", 10);
    store->update_page_status("https://example.com/page2", updated);

    int requeued = 0;
    store->requeue_incomplete([&requeued](WorkItem item) {
        ++requeued;
        EXPECT_FALSE(item.url.empty());
    });

    EXPECT_EQ(requeued, 1);
}

TEST_F(RocksDBStoreTest, WriteVisitedLog) {
    store->mark_page_visited("https://example.com/page1");
    std::string log_path = "./visited_log.tsv";

    store->write_visited_log(log_path);
    EXPECT_TRUE(std::filesystem::exists(log_path));
    std::filesystem::remove(log_path);
}
