#include <gtest/gtest.h>
#include "scrape_llm/validator.hpp"
#include "scrape_llm/llm_client.hpp"
#include <nlohmann/json.hpp>

namespace {

class MockRepairClient : public scrapellm::ILlmClient {
public:
    nlohmann::json repaired_record;
    std::optional<std::string> chat(const std::string&, const std::string&) override {
        return repaired_record.dump();
    }
};

} // namespace

TEST(Validator, ValidRecordPasses) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {{"name", {{"type", "string"}}}, {"count", {{"type", "integer"}}}}},
        {"required", nlohmann::json::array({"name"})}
    };
    nlohmann::json record = {{"name", "test"}, {"count", 42}};
    auto result = scrapellm::validate_record(record, schema);
    EXPECT_TRUE(result.valid);
}

TEST(Validator, MissingRequiredFails) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {{"name", {{"type", "string"}}}}},
        {"required", nlohmann::json::array({"name"})}
    };
    nlohmann::json record = {{"other", "x"}};
    auto result = scrapellm::validate_record(record, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.error_message.find("name") != std::string::npos);
}

TEST(Validator, RepairReturnsValidRecord) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {{"name", {{"type", "string"}}}, {"value", {{"type", "number"}}}}},
        {"required", nlohmann::json::array({"name"})}
    };
    nlohmann::json invalid_record = {{"name", 123}};  // wrong type
    MockRepairClient mock;
    mock.repaired_record = {{"name", "repaired"}, {"value", 1.5}};
    auto repaired = scrapellm::repair_record(mock, invalid_record, schema, "expected string");
    ASSERT_TRUE(repaired.has_value());
    auto vr = scrapellm::validate_record(*repaired, schema);
    EXPECT_TRUE(vr.valid);
    EXPECT_EQ((*repaired)["name"], "repaired");
}
