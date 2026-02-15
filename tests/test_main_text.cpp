#include <gtest/gtest.h>
#include "parse/html_parser.hpp"
#include "scrape_llm/content_extractor.hpp"
#include <string>

TEST(MainText, StripsScriptAndStyle) {
    std::string html = R"(<!DOCTYPE html><html><head><title>Page</title></head><body>
        <p>Hello world</p>
        <script>alert("x"); var y = 1;</script>
        <style>.x { color: red; }</style>
        <p>More content here</p>
    </body></html>)";
    docscraper::parse::HTMLDocument doc(html);
    std::string main = doc.main_text();
    EXPECT_TRUE(main.find("Hello world") != std::string::npos);
    EXPECT_TRUE(main.find("More content here") != std::string::npos);
    EXPECT_TRUE(main.find("alert") == std::string::npos);
    EXPECT_TRUE(main.find("color: red") == std::string::npos);
}

TEST(MainText, ExtractedContentHasMainText) {
    std::string html = R"(<!DOCTYPE html><html><head><title>Title</title></head><body>
        <h1>Heading</h1>
        <p>Paragraph text</p>
    </body></html>)";
    docscraper::parse::HTMLDocument doc(html);
    auto content = scrapellm::extract_content(doc, "https://example.com/");
    EXPECT_TRUE(content.main_text.find("Paragraph text") != std::string::npos);
    EXPECT_EQ(content.title, "Title");
}
