// test_markdown_converter.cpp - Unit tests for Markdown converter
#include <gtest/gtest.h>
#include "process/markdown_converter.hpp"

using namespace docscraper::process;

TEST(MarkdownConverterTest, HeadingsAndParagraphs) {
    MarkdownConverter converter;
    std::string html = "<h1>Title</h1><p>Hello <strong>World</strong></p>";
    std::string md = converter.convert(html);

    EXPECT_NE(md.find("# Title"), std::string::npos);
    EXPECT_NE(md.find("Hello **World**"), std::string::npos);
}
