// test_chunker.cpp - Unit tests for chunker
#include <gtest/gtest.h>
#include "process/chunker.hpp"

using namespace docscraper::process;

TEST(ChunkerTest, SplitByHeadings) {
    ChunkerConfig config;
    Chunker chunker(config);

    std::string markdown = R"(
# Title
Intro text.
## Section 1
Content here.
## Section 2
More content.
)";

    auto chunks = chunker.chunk_markdown(markdown);
    EXPECT_FALSE(chunks.empty());
    EXPECT_GE(chunks.size(), 2);
}
