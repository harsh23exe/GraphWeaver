#pragma once

#include <string>
#include <functional>
#include <optional>

namespace scrapellm {

// Abstract LLM client for tests (mock) and production (HTTP).
class ILlmClient {
public:
    virtual ~ILlmClient() = default;

    // Send a chat request; system_prompt optional. Returns response body or nullopt on failure.
    virtual std::optional<std::string> chat(
        const std::string& user_message,
        const std::string& system_prompt = ""
    ) = 0;

    // Optional: request JSON-only response (if API supports it).
    virtual void set_json_mode(bool on) { (void)on; }
};

// Production client: libcurl, base_url, API key from env, retries with backoff.
class LlmClient : public ILlmClient {
public:
    LlmClient(std::string base_url, std::string model, std::string api_key_env = "GEMINI_API_KEY");

    std::optional<std::string> chat(
        const std::string& user_message,
        const std::string& system_prompt = ""
    ) override;

    void set_json_mode(bool on) override { json_mode_ = on; }
    void set_max_tokens(int n) { max_tokens_ = n; }

    std::string get_api_key() const;

private:
    std::string base_url_;
    std::string model_;
    std::string api_key_env_;
    bool json_mode_ = false;
    int max_tokens_ = 4096;
};

} // namespace scrapellm
