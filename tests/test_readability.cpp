// test_readability.cpp - Unit tests for readability fallback
#include <gtest/gtest.h>
#include "detect/readability.hpp"

using namespace docscraper::detect;
using namespace docscraper::parse;

TEST(ReadabilityTest, ExtractFromArticle) {
    std::string html = R"(
<html><body>
<article><h1>Title</h1><p>Hello world</p></article>
</body></html>
)";
    HTMLDocument doc(html);
    ReadabilityExtractor extractor;
    auto result = extractor.extract(doc);
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.content.find("Hello"), std::string::npos);
}

TEST(ReadabilityTest, ExtractFromBody) {
    std::string html = R"(
<html><body><p>Body content</p></body></html>
)";
    HTMLDocument doc(html);
    ReadabilityExtractor extractor;
    auto result = extractor.extract(doc);
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.content.find("Body"), std::string::npos);
}
