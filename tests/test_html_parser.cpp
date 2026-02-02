// test_html_parser.cpp - Unit tests for HTML parser
#include <gtest/gtest.h>
#include "parse/html_parser.hpp"

using namespace docscraper::parse;

TEST(HTMLParserTest, SelectByTag) {
    std::string html = R"(
<html><body><main><h1>Title</h1></main></body></html>
)";
    HTMLDocument doc(html);
    auto main = doc.select_first("main");
    ASSERT_TRUE(main.has_value());
    EXPECT_EQ(main->tag_name(), "main");
}

TEST(HTMLParserTest, SelectByClass) {
    std::string html = R"(
<div class="content"><p>Hello</p></div>
)";
    HTMLDocument doc(html);
    auto content = doc.select_first(".content");
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->has_class("content"));
}

TEST(HTMLParserTest, SelectById) {
    std::string html = R"(
<section id="docs">Docs</section>
)";
    HTMLDocument doc(html);
    auto docs = doc.select_first("#docs");
    ASSERT_TRUE(docs.has_value());
    EXPECT_TRUE(docs->has_id("docs"));
}

TEST(HTMLParserTest, TextExtraction) {
    std::string html = R"(
<div class="content"><p>Hello <strong>World</strong></p></div>
)";
    HTMLDocument doc(html);
    auto content = doc.select_first(".content");
    ASSERT_TRUE(content.has_value());
    std::string text = content->text();
    EXPECT_NE(text.find("Hello"), std::string::npos);
    EXPECT_NE(text.find("World"), std::string::npos);
}
