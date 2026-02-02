// test_sitemap_parser.cpp - Unit tests for sitemap parser
#include <gtest/gtest.h>
#include "parse/sitemap_parser.hpp"

using namespace docscraper::parse;

TEST(SitemapParserTest, ParseURLSet) {
    std::string xml = R"(
<urlset>
  <url><loc>https://example.com/a</loc><lastmod>2024-01-01</lastmod></url>
  <url><loc>https://example.com/b</loc></url>
</urlset>
)";
    SitemapParser parser;
    auto urls = parser.parse_urlset(xml);
    ASSERT_EQ(urls.size(), 2);
    EXPECT_EQ(urls[0].loc, "https://example.com/a");
    EXPECT_EQ(urls[1].loc, "https://example.com/b");
}

TEST(SitemapParserTest, ParseIndex) {
    std::string xml = R"(
<sitemapindex>
  <sitemap><loc>https://example.com/sitemap1.xml</loc></sitemap>
  <sitemap><loc>https://example.com/sitemap2.xml</loc></sitemap>
</sitemapindex>
)";
    SitemapParser parser;
    auto sitemaps = parser.parse_index(xml);
    ASSERT_EQ(sitemaps.size(), 2);
    EXPECT_EQ(sitemaps[0].loc, "https://example.com/sitemap1.xml");
}

TEST(SitemapParserTest, DetectType) {
    SitemapParser parser;
    EXPECT_EQ(parser.detect_type("<urlset></urlset>"), SitemapParser::SitemapType::URLSet);
    EXPECT_EQ(parser.detect_type("<sitemapindex></sitemapindex>"), SitemapParser::SitemapType::Index);
}
