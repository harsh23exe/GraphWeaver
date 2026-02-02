// test_normalizer.cpp - Unit tests for URL normalizer
#include <gtest/gtest.h>
#include "parse/normalizer.hpp"

using namespace docscraper::parse;

// ============================================================================
// URL Parsing Tests
// ============================================================================

TEST(URLParserTest, BasicParsing) {
    auto result = URLNormalizer::parse("https://example.com/path/to/page");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->scheme, "https");
    EXPECT_EQ(result->host, "example.com");
    EXPECT_EQ(result->path, "/path/to/page");
}

TEST(URLParserTest, WithPort) {
    auto result = URLNormalizer::parse("http://example.com:8080/page");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->port, 8080);
    EXPECT_EQ(result->effective_port(), 8080);
}

TEST(URLParserTest, WithQuery) {
    auto result = URLNormalizer::parse("https://example.com/search?q=test&page=1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->path, "/search");
    EXPECT_EQ(result->query, "q=test&page=1");
}

TEST(URLParserTest, WithFragment) {
    auto result = URLNormalizer::parse("https://example.com/docs#section-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->path, "/docs");
    EXPECT_EQ(result->fragment, "section-1");
}

TEST(URLParserTest, CompleteURL) {
    auto result = URLNormalizer::parse("https://example.com:443/path?query=val#frag");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->scheme, "https");
    EXPECT_EQ(result->host, "example.com");
    EXPECT_EQ(result->port, 443);
    EXPECT_EQ(result->path, "/path");
    EXPECT_EQ(result->query, "query=val");
    EXPECT_EQ(result->fragment, "frag");
}

TEST(URLParserTest, InvalidURL) {
    auto result = URLNormalizer::parse("not-a-url");
    EXPECT_FALSE(result.has_value());
}

TEST(URLParserTest, EmptyURL) {
    auto result = URLNormalizer::parse("");
    EXPECT_FALSE(result.has_value());
}

TEST(URLParserTest, FTPProtocolNotSupported) {
    auto result = URLNormalizer::parse("ftp://example.com/file");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// URL Normalization Tests
// ============================================================================

TEST(URLNormalizerTest, LowercaseSchemeAndHost) {
    auto result = URLNormalizer::normalize("HTTPS://EXAMPLE.COM/Path");
    EXPECT_EQ(result, "https://example.com/Path");
}

TEST(URLNormalizerTest, StripFragment) {
    auto result = URLNormalizer::normalize("https://example.com/page#section");
    EXPECT_EQ(result, "https://example.com/page");
}

TEST(URLNormalizerTest, KeepFragment) {
    auto result = URLNormalizer::normalize("https://example.com/page#section", true);
    EXPECT_EQ(result, "https://example.com/page#section");
}

TEST(URLNormalizerTest, RemoveDefaultPortHTTPS) {
    auto result = URLNormalizer::normalize("https://example.com:443/path");
    EXPECT_EQ(result, "https://example.com/path");
}

TEST(URLNormalizerTest, RemoveDefaultPortHTTP) {
    auto result = URLNormalizer::normalize("http://example.com:80/path");
    EXPECT_EQ(result, "http://example.com/path");
}

TEST(URLNormalizerTest, KeepNonDefaultPort) {
    auto result = URLNormalizer::normalize("https://example.com:8443/path");
    EXPECT_EQ(result, "https://example.com:8443/path");
}

TEST(URLNormalizerTest, SortQueryParams) {
    auto result = URLNormalizer::normalize("https://example.com/search?z=last&a=first&m=middle");
    EXPECT_EQ(result, "https://example.com/search?a=first&m=middle&z=last");
}

TEST(URLNormalizerTest, NormalizePath) {
    auto result = URLNormalizer::normalize("https://example.com/a/b/../c/./d");
    EXPECT_EQ(result, "https://example.com/a/c/d");
}

TEST(URLNormalizerTest, EmptyPath) {
    auto result = URLNormalizer::normalize("https://example.com");
    EXPECT_EQ(result, "https://example.com/");
}

// ============================================================================
// Scope Checking Tests
// ============================================================================

TEST(URLScopeTest, ExactDomainMatch) {
    EXPECT_TRUE(URLNormalizer::is_in_scope(
        "https://example.com/page",
        "example.com"
    ));
}

TEST(URLScopeTest, SubdomainMatch) {
    EXPECT_TRUE(URLNormalizer::is_in_scope(
        "https://docs.example.com/page",
        "example.com"
    ));
}

TEST(URLScopeTest, DifferentDomain) {
    EXPECT_FALSE(URLNormalizer::is_in_scope(
        "https://other.com/page",
        "example.com"
    ));
}

TEST(URLScopeTest, PathPrefixMatch) {
    EXPECT_TRUE(URLNormalizer::is_in_scope(
        "https://example.com/docs/api/reference",
        "example.com",
        "/docs/"
    ));
}

TEST(URLScopeTest, PathPrefixNoMatch) {
    EXPECT_FALSE(URLNormalizer::is_in_scope(
        "https://example.com/blog/post",
        "example.com",
        "/docs/"
    ));
}

TEST(URLScopeTest, CaseInsensitiveDomain) {
    EXPECT_TRUE(URLNormalizer::is_in_scope(
        "https://EXAMPLE.COM/page",
        "example.com"
    ));
}

// ============================================================================
// URL Resolution Tests
// ============================================================================

TEST(URLResolveTest, AbsoluteURL) {
    auto result = URLNormalizer::resolve(
        "https://example.com/docs/",
        "https://other.com/page"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "https://other.com/page");
}

TEST(URLResolveTest, RelativePath) {
    auto result = URLNormalizer::resolve(
        "https://example.com/docs/guide/",
        "intro.html"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "https://example.com/docs/guide/intro.html");
}

TEST(URLResolveTest, ParentDirectory) {
    auto result = URLNormalizer::resolve(
        "https://example.com/docs/guide/page.html",
        "../images/logo.png"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "https://example.com/docs/images/logo.png");
}

TEST(URLResolveTest, RootRelative) {
    auto result = URLNormalizer::resolve(
        "https://example.com/docs/guide/",
        "/assets/style.css"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "https://example.com/assets/style.css");
}

TEST(URLResolveTest, QueryOnly) {
    auto result = URLNormalizer::resolve(
        "https://example.com/search",
        "?q=test"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "https://example.com/search?q=test");
}

TEST(URLResolveTest, ProtocolRelative) {
    auto result = URLNormalizer::resolve(
        "https://example.com/docs/",
        "//cdn.example.com/script.js"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "https://cdn.example.com/script.js");
}

// ============================================================================
// Helper Functions Tests
// ============================================================================

TEST(URLHelperTest, ExtractDomain) {
    EXPECT_EQ(URLNormalizer::extract_domain("https://example.com/path"), "example.com");
    EXPECT_EQ(URLNormalizer::extract_domain("http://sub.domain.com:8080/"), "sub.domain.com");
    EXPECT_EQ(URLNormalizer::extract_domain("invalid"), "");
}

TEST(URLHelperTest, ExtractPath) {
    EXPECT_EQ(URLNormalizer::extract_path("https://example.com/path/to/page"), "/path/to/page");
    EXPECT_EQ(URLNormalizer::extract_path("https://example.com"), "/");
}

TEST(URLHelperTest, IsAbsolute) {
    EXPECT_TRUE(URLNormalizer::is_absolute("https://example.com"));
    EXPECT_TRUE(URLNormalizer::is_absolute("http://example.com"));
    // Protocol-relative URLs are NOT considered absolute (they need a base URL's scheme)
    EXPECT_FALSE(URLNormalizer::is_absolute("//cdn.example.com"));
    EXPECT_FALSE(URLNormalizer::is_absolute("/path/to/page"));
    EXPECT_FALSE(URLNormalizer::is_absolute("relative/path"));
}

TEST(URLHelperTest, IsValidHttpUrl) {
    EXPECT_TRUE(URLNormalizer::is_valid_http_url("https://example.com"));
    EXPECT_TRUE(URLNormalizer::is_valid_http_url("http://example.com"));
    EXPECT_FALSE(URLNormalizer::is_valid_http_url("ftp://example.com"));
    EXPECT_FALSE(URLNormalizer::is_valid_http_url("not-a-url"));
}

TEST(URLHelperTest, GetExtension) {
    EXPECT_EQ(URLNormalizer::get_extension("https://example.com/file.pdf"), "pdf");
    EXPECT_EQ(URLNormalizer::get_extension("https://example.com/page.html"), "html");
    EXPECT_EQ(URLNormalizer::get_extension("https://example.com/path/"), "");
    EXPECT_EQ(URLNormalizer::get_extension("https://example.com/file.tar.gz"), "gz");
}

TEST(URLHelperTest, UrlEncode) {
    EXPECT_EQ(URLNormalizer::url_encode("hello world"), "hello%20world");
    EXPECT_EQ(URLNormalizer::url_encode("a=b&c=d"), "a%3Db%26c%3Dd");
    EXPECT_EQ(URLNormalizer::url_encode("safe-chars_123.test~"), "safe-chars_123.test~");
}

TEST(URLHelperTest, UrlDecode) {
    EXPECT_EQ(URLNormalizer::url_decode("hello%20world"), "hello world");
    EXPECT_EQ(URLNormalizer::url_decode("a%3Db%26c%3Dd"), "a=b&c=d");
    EXPECT_EQ(URLNormalizer::url_decode("hello+world"), "hello world");
}

// ============================================================================
// url_to_filepath Tests
// ============================================================================

TEST(UrlToFilepathTest, BasicConversion) {
    auto result = url_to_filepath(
        "https://example.com/docs/guide",
        "example.com",
        "./output"
    );
    EXPECT_EQ(result, "./output/example.com/docs/guide.md");
}

TEST(UrlToFilepathTest, HtmlExtension) {
    auto result = url_to_filepath(
        "https://example.com/docs/page.html",
        "example.com",
        ""
    );
    EXPECT_EQ(result, "example.com/docs/page.md");
}

TEST(UrlToFilepathTest, RootPath) {
    auto result = url_to_filepath(
        "https://example.com/",
        "example.com",
        ""
    );
    EXPECT_EQ(result, "example.com/index.md");
}
