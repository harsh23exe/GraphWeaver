// test_placeholder.cpp - Placeholder test to verify test infrastructure
#include <gtest/gtest.h>

TEST(PlaceholderTest, VerifyTestInfrastructure) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(PlaceholderTest, StringComparison) {
    std::string expected = "doc-scraper";
    std::string actual = "doc-scraper";
    EXPECT_EQ(expected, actual);
}
