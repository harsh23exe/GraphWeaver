// test_fetcher.cpp - Unit tests for HTTP fetcher
#include <gtest/gtest.h>
#include "fetch/fetcher.hpp"
#include <httplib.h>
#include <thread>
#include <atomic>

using namespace docscraper::fetch;
using namespace docscraper::crawler;

class TestServer {
public:
    TestServer() : port_(8085) {}

    void start() {
        server_.Get("/ok", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("ok", "text/plain");
            res.status = 200;
        });

        server_.Get("/html", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("<html><body>hi</body></html>", "text/html");
            res.status = 200;
        });

        server_.Get("/retry", [this](const httplib::Request&, httplib::Response& res) {
            int count = retry_count_.fetch_add(1);
            if (count < 2) {
                res.status = 500;
                res.set_content("error", "text/plain");
            } else {
                res.status = 200;
                res.set_content("ok", "text/plain");
            }
        });

        thread_ = std::thread([this]() {
            server_.listen("localhost", port_);
        });

        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void stop() {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    int port() const { return port_; }
    void reset_retry_count() { retry_count_.store(0); }

private:
    httplib::Server server_;
    std::thread thread_;
    int port_;
    std::atomic<int> retry_count_{0};
};

TEST(FetcherTest, BasicFetch) {
    TestServer server;
    server.start();

    AppConfig config;
    config.max_retries = 1;
    Fetcher fetcher(config);

    auto result = fetcher.fetch_once("http://localhost:8085/ok");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.status_code, 200);
    EXPECT_EQ(result.body, "ok");

    server.stop();
}

TEST(FetcherTest, HTMLContentType) {
    TestServer server;
    server.start();

    AppConfig config;
    Fetcher fetcher(config);

    auto result = fetcher.fetch_once("http://localhost:8085/html");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.content_type.find("text/html"), std::string::npos);

    server.stop();
}

TEST(FetcherTest, RetryBehavior) {
    TestServer server;
    server.start();
    server.reset_retry_count();

    AppConfig config;
    config.max_retries = 3;
    config.initial_retry_delay = std::chrono::seconds(0);
    config.max_retry_delay = std::chrono::seconds(0);

    Fetcher fetcher(config);
    auto result = fetcher.fetch_with_retry("http://localhost:8085/retry");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.status_code, 200);

    server.stop();
}
