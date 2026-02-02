// test_utils.cpp - Unit tests for utilities
#include <gtest/gtest.h>
#include "utils/hash.hpp"
#include "utils/sanitize.hpp"
#include "utils/errors.hpp"

using namespace docscraper::utils;

// ============================================================================
// Hash Tests
// ============================================================================

TEST(HashTest, MD5Empty) {
    auto result = md5_hash("");
    EXPECT_EQ(result, "d41d8cd98f00b204e9800998ecf8427e");
}

TEST(HashTest, MD5HelloWorld) {
    auto result = md5_hash("Hello, World!");
    EXPECT_EQ(result, "65a8e27d8879283831b664bd8b7f0ad4");
}

TEST(HashTest, SHA256Empty) {
    auto result = sha256_hash("");
    EXPECT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(HashTest, SHA256HelloWorld) {
    auto result = sha256_hash("Hello, World!");
    EXPECT_EQ(result, "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f");
}

TEST(HashTest, SHA1Empty) {
    auto result = sha1_hash("");
    EXPECT_EQ(result, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST(HashTest, ContentHash) {
    auto hash1 = content_hash("Some content");
    auto hash2 = content_hash("Some content");
    auto hash3 = content_hash("Different content");
    
    EXPECT_EQ(hash1, hash2);
    EXPECT_NE(hash1, hash3);
}

TEST(HashTest, URLHash) {
    auto hash = url_hash("https://example.com/page");
    EXPECT_EQ(hash.length(), 32);  // MD5 produces 32 hex chars
}

TEST(HashTest, BytesToHex) {
    unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_EQ(bytes_to_hex(data, 4), "deadbeef");
}

TEST(HashTest, HexToBytes) {
    auto bytes = hex_to_bytes("deadbeef");
    ASSERT_EQ(bytes.size(), 4);
    EXPECT_EQ(bytes[0], 0xDE);
    EXPECT_EQ(bytes[1], 0xAD);
    EXPECT_EQ(bytes[2], 0xBE);
    EXPECT_EQ(bytes[3], 0xEF);
}

// ============================================================================
// Sanitize Tests
// ============================================================================

TEST(SanitizeTest, FilenameBasic) {
    EXPECT_EQ(sanitize_filename("hello.txt"), "hello.txt");
    EXPECT_EQ(sanitize_filename("my file.doc"), "my file.doc");
}

TEST(SanitizeTest, FilenameInvalidChars) {
    EXPECT_EQ(sanitize_filename("file<name>.txt"), "file_name_.txt");
    EXPECT_EQ(sanitize_filename("path/to/file"), "path_to_file");
    EXPECT_EQ(sanitize_filename("name:with:colons"), "name_with_colons");
}

TEST(SanitizeTest, FilenameReservedNames) {
    EXPECT_EQ(sanitize_filename("CON"), "_CON");
    EXPECT_EQ(sanitize_filename("NUL.txt"), "_NUL.txt");
    EXPECT_EQ(sanitize_filename("aux"), "_aux");  // Case insensitive
}

TEST(SanitizeTest, FilenameEmpty) {
    EXPECT_EQ(sanitize_filename(""), "unnamed");
    EXPECT_EQ(sanitize_filename("..."), "unnamed");
}

TEST(SanitizeTest, FilenameTruncate) {
    std::string long_name(300, 'a');
    auto result = sanitize_filename(long_name);
    EXPECT_LE(result.length(), 255);
}

TEST(SanitizeTest, PathBasic) {
    EXPECT_EQ(sanitize_path("/path/to/file.txt"), "/path/to/file.txt");
    EXPECT_EQ(sanitize_path("relative/path"), "relative/path");
}

TEST(SanitizeTest, PathNormalization) {
    EXPECT_EQ(sanitize_path("/path/../other"), "/other");
    EXPECT_EQ(sanitize_path("/path/./file"), "/path/file");
}

TEST(SanitizeTest, CollapseDuplicates) {
    EXPECT_EQ(collapse_duplicates("a___b", '_'), "a_b");
    EXPECT_EQ(collapse_duplicates("---", '-'), "-");
    EXPECT_EQ(collapse_duplicates("no_dupes", '_'), "no_dupes");
}

TEST(SanitizeTest, TruncateUTF8) {
    // ASCII
    EXPECT_EQ(truncate_utf8("hello", 3), "hel");
    
    // UTF-8 (don't cut in middle of character)
    std::string utf8 = "héllo";  // 'é' is 2 bytes
    auto result = truncate_utf8(utf8, 3);
    // Should not cut in middle of 'é'
    EXPECT_LE(result.length(), 3);
}

TEST(SanitizeTest, StripHTMLTags) {
    EXPECT_EQ(strip_html_tags("<p>Hello</p>"), "Hello");
    EXPECT_EQ(strip_html_tags("<a href='url'>link</a>"), "link");
    EXPECT_EQ(strip_html_tags("no tags"), "no tags");
}

TEST(SanitizeTest, NormalizeWhitespace) {
    EXPECT_EQ(normalize_whitespace("  hello   world  "), "hello world");
    EXPECT_EQ(normalize_whitespace("line\n\nbreak"), "line break");
    EXPECT_EQ(normalize_whitespace("\t\ttabs\t\t"), "tabs");
}

TEST(SanitizeTest, EscapeTSV) {
    EXPECT_EQ(escape_tsv("col1\tcol2"), "col1\\tcol2");
    EXPECT_EQ(escape_tsv("line1\nline2"), "line1\\nline2");
}

TEST(SanitizeTest, EscapeYAML) {
    EXPECT_EQ(escape_yaml("plain"), "plain");
    EXPECT_EQ(escape_yaml("with: colon"), "\"with: colon\"");
    EXPECT_EQ(escape_yaml("has \"quotes\""), "\"has \\\"quotes\\\"\"");
}

TEST(SanitizeTest, IsValidUTF8) {
    EXPECT_TRUE(is_valid_utf8("Hello"));
    EXPECT_TRUE(is_valid_utf8("Héllo"));
    EXPECT_TRUE(is_valid_utf8("你好"));
    EXPECT_FALSE(is_valid_utf8("\xFF\xFE"));  // Invalid bytes
}

TEST(SanitizeTest, Slugify) {
    EXPECT_EQ(slugify("Hello World!"), "hello-world");
    EXPECT_EQ(slugify("This is a TEST"), "this-is-a-test");
    EXPECT_EQ(slugify("  multiple   spaces  "), "multiple-spaces");
    EXPECT_EQ(slugify("!!!"), "untitled");
}

TEST(SanitizeTest, JoinPath) {
    EXPECT_EQ(join_path("a", "b"), "a/b");
    EXPECT_EQ(join_path("a/", "b"), "a/b");
    EXPECT_EQ(join_path("a", "/b"), "a/b");
    EXPECT_EQ(join_path("a/", "/b"), "a/b");
}

TEST(SanitizeTest, SplitPath) {
    auto [dir, file] = split_path("/path/to/file.txt");
    EXPECT_EQ(dir, "/path/to");
    EXPECT_EQ(file, "file.txt");
}

TEST(SanitizeTest, ParentDirectory) {
    EXPECT_EQ(parent_directory("/path/to/file.txt"), "/path/to");
    EXPECT_EQ(parent_directory("file.txt"), "");
}

// ============================================================================
// Error Tests
// ============================================================================

TEST(ErrorTest, CrawlerError) {
    CrawlerError error("test error");
    EXPECT_EQ(std::string(error.what()), "test error");
}

TEST(ErrorTest, ScopeViolationError) {
    ScopeViolationError error("https://bad.com", "wrong domain");
    EXPECT_NE(std::string(error.what()).find("bad.com"), std::string::npos);
    EXPECT_EQ(error.url(), "https://bad.com");
    EXPECT_EQ(error.reason(), "wrong domain");
}

TEST(ErrorTest, HTTPError) {
    HTTPError error(404, "Not Found");
    EXPECT_EQ(error.status_code(), 404);
    EXPECT_NE(std::string(error.what()).find("404"), std::string::npos);
}

TEST(ErrorTest, MaxRetriesError) {
    MaxRetriesError error("https://example.com", 5);
    EXPECT_EQ(error.url(), "https://example.com");
    EXPECT_EQ(error.attempts(), 5);
}

TEST(ErrorTest, ContentSelectorError) {
    ContentSelectorError error(".missing", "https://example.com");
    EXPECT_EQ(error.selector(), ".missing");
    EXPECT_EQ(error.url(), "https://example.com");
}
