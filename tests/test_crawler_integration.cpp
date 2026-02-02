// test_crawler_integration.cpp - Basic crawler integration test
#include <gtest/gtest.h>
#include "crawler/crawler.hpp"
#include "fetch/fetcher.hpp"
#include "fetch/rate_limiter.hpp"
#include "storage/rocksdb_store.hpp"
#include <httplib.h>
#include <filesystem>
#include <thread>

using namespace docscraper;

class LocalTestServer {
public:
    LocalTestServer() : port_(8086) {}

    void start() {
        server_.Get("/docs", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"(
<html><body><main>
<h1>Docs Home</h1>
<a href="/docs/page1">Page 1</a>
</main></body></html>
)", "text/html");
            res.status = 200;
        });

        server_.Get("/docs/page1", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"(
<html><body><main>
<h1>Page 1</h1>
<p>Content here.</p>
</main></body></html>
)", "text/html");
            res.status = 200;
        });

        thread_ = std::thread([this]() {
            server_.listen("localhost", port_);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void stop() {
        server_.stop();
        if (thread_.joinable()) thread_.join();
    }

private:
    httplib::Server server_;
    std::thread thread_;
    int port_;
};

TEST(CrawlerIntegrationTest, BasicCrawl) {
    LocalTestServer server;
    server.start();

    crawler::AppConfig app_config;
    app_config.num_workers = 2;
    app_config.output_base_dir = "./test_crawl_output";
    app_config.state_dir = "./test_crawl_state";

    crawler::SiteConfig site_config;
    site_config.start_urls = {"http://localhost:8086/docs"};
    site_config.allowed_domain = "localhost";
    site_config.allowed_path_prefix = "/docs";
    site_config.content_selector = "main";
    site_config.max_depth = 2;

    storage::RocksDBStore store(app_config.state_dir, site_config.allowed_domain, false);
    fetch::Fetcher fetcher(app_config);
    fetch::RateLimiter limiter(std::chrono::milliseconds(10));

    crawler::Crawler crawler_engine(app_config, site_config, "test_site", store, fetcher, limiter, false);
    crawler_engine.run();

    EXPECT_GT(crawler_engine.get_pages_processed(), 0);

    // Validate output files exist
    std::filesystem::path output_dir(app_config.output_base_dir);
    EXPECT_TRUE(std::filesystem::exists(output_dir));

    server.stop();
    std::filesystem::remove_all(app_config.output_base_dir);
    std::filesystem::remove_all(app_config.state_dir);
}
