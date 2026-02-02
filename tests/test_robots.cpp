// test_robots.cpp - Unit tests for robots.txt parser
#include <gtest/gtest.h>
#include "fetch/robots.hpp"

using namespace docscraper::fetch;

TEST(RobotsTest, BasicParsing) {
    std::string robots_txt = R"(
User-agent: *
Disallow: /private
Allow: /public
Sitemap: https://example.com/sitemap.xml
)";

    RobotsHandler handler;
    handler.parse(robots_txt);

    EXPECT_FALSE(handler.is_allowed("/private/secret", "*"));
    EXPECT_TRUE(handler.is_allowed("/public/page", "*"));
    EXPECT_TRUE(handler.is_allowed("/other", "*"));

    auto sitemaps = handler.get_sitemaps();
    ASSERT_EQ(sitemaps.size(), 1);
    EXPECT_EQ(sitemaps[0], "https://example.com/sitemap.xml");
}

TEST(RobotsTest, UserAgentSpecific) {
    std::string robots_txt = R"(
User-agent: googlebot
Disallow: /nogoogle

User-agent: *
Disallow: /private
)";

    RobotsHandler handler;
    handler.parse(robots_txt);

    // googlebot-specific rule
    EXPECT_FALSE(handler.is_allowed("/nogoogle/page", "googlebot"));
    EXPECT_TRUE(handler.is_allowed("/private/page", "googlebot"));  // not in googlebot rules

    // wildcard applies to others
    EXPECT_FALSE(handler.is_allowed("/private/page", "otherbot"));
}

TEST(RobotsTest, AllowOverridesDisallow) {
    std::string robots_txt = R"(
User-agent: *
Disallow: /docs
Allow: /docs/public
)";

    RobotsHandler handler;
    handler.parse(robots_txt);

    EXPECT_FALSE(handler.is_allowed("/docs/private", "*"));
    EXPECT_TRUE(handler.is_allowed("/docs/public/page", "*"));
}
