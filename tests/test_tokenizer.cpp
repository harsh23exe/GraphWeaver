// test_tokenizer.cpp - Unit tests for tokenizer
#include <gtest/gtest.h>
#include "process/tokenizer.hpp"

using namespace docscraper::process;

TEST(TokenizerTest, BasicCounting) {
    Tokenizer tokenizer;
    EXPECT_GT(tokenizer.count_tokens("Hello world"), 0);
    EXPECT_LT(tokenizer.count_tokens("Hello"), tokenizer.count_tokens("Hello world"));
}
