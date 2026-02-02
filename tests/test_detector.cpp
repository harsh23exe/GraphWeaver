// test_detector.cpp - Unit tests for framework detector
#include <gtest/gtest.h>
#include "detect/detector.hpp"

using namespace docscraper::detect;
using namespace docscraper::parse;

TEST(DetectorTest, DocusaurusDetection) {
    std::string html = R"(
<html>
<head><meta name="generator" content="Docusaurus"/></head>
<body><main class="mainContainer">Docs</main></body>
</html>
)";
    HTMLDocument doc(html);
    ContentDetector detector;
    auto result = detector.detect(doc, "https://example.com/docs");
    EXPECT_EQ(result.framework, Framework::Docusaurus);
    EXPECT_FALSE(result.selector.empty());
}

TEST(DetectorTest, UnknownFallback) {
    std::string html = R"(<html><body><p>Plain</p></body></html>)";
    HTMLDocument doc(html);
    ContentDetector detector;
    auto result = detector.detect(doc, "https://example.com/docs");
    EXPECT_TRUE(result.fallback);
    EXPECT_FALSE(result.selector.empty());
}
