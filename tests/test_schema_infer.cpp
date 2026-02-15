#include <gtest/gtest.h>
#include "scrape_llm/schema_infer.hpp"
#include "scrape_llm/llm_client.hpp"
#include <string>

namespace {

class MockLlmClient : public scrapellm::ILlmClient {
public:
    std::optional<std::string> response;
    std::optional<std::string> chat(const std::string&, const std::string&) override {
        return response;
    }
};

} // namespace

TEST(SchemaInfer, ValidResponseParsed) {
    MockLlmClient mock;
    mock.response = R"({
        "json_schema": {"type": "object", "properties": {"name": {"type": "string"}}, "required": ["name"]},
        "extraction_mode": "list",
        "hints": {"dedupe_key": "name"}
    })";
    std::string warning;
    auto schema = scrapellm::schema_infer(mock, "extract names", warning);
    EXPECT_TRUE(warning.empty());
    EXPECT_EQ(schema.extraction_mode, "list");
    EXPECT_TRUE(schema.json_schema.contains("type"));
    EXPECT_EQ(schema.json_schema["type"], "object");
    EXPECT_TRUE(schema.hints.contains("dedupe_key"));
    EXPECT_EQ(schema.hints["dedupe_key"], "name");
}

TEST(SchemaInfer, NullResponseUsesFallback) {
    MockLlmClient mock;
    mock.response = std::nullopt;
    std::string warning;
    auto schema = scrapellm::schema_infer(mock, "extract names", warning);
    EXPECT_FALSE(warning.empty());
    EXPECT_EQ(schema.extraction_mode, "list");
    EXPECT_TRUE(schema.json_schema.contains("properties"));
    EXPECT_TRUE(schema.json_schema["properties"].contains("source_url"));
    EXPECT_TRUE(schema.json_schema["properties"].contains("content"));
}

TEST(SchemaInfer, InvalidJsonUsesFallback) {
    MockLlmClient mock;
    mock.response = "not valid json at all";
    std::string warning;
    auto schema = scrapellm::schema_infer(mock, "extract names", warning);
    EXPECT_FALSE(warning.empty());
    EXPECT_TRUE(schema.json_schema.contains("properties"));
    EXPECT_TRUE(schema.json_schema["properties"].contains("source_url"));
}

TEST(SchemaInfer, MissingFieldsUsesFallback) {
    MockLlmClient mock;
    mock.response = R"({"extraction_mode": "single"})";
    std::string warning;
    auto schema = scrapellm::schema_infer(mock, "extract names", warning);
    EXPECT_FALSE(warning.empty());
    EXPECT_TRUE(schema.json_schema.contains("properties"));
    EXPECT_TRUE(schema.json_schema["properties"].contains("source_url"));
}
