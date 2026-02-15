#include "scrape_llm/llm_client.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace scrapellm {

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* str = static_cast<std::string*>(userdata);
    str->append(ptr, size * nmemb);
    return size * nmemb;
}

LlmClient::LlmClient(std::string base_url, std::string model, std::string api_key_env)
    : base_url_(std::move(base_url))
    , model_(std::move(model))
    , api_key_env_(std::move(api_key_env))
{}

std::string LlmClient::get_api_key() const {
    const char* v = std::getenv(api_key_env_.c_str());
    if (!v || !*v) return "";
    return std::string(v);
}

std::optional<std::string> LlmClient::chat(const std::string& user_message, const std::string& system_prompt) {
    std::string key = get_api_key();
    if (key.empty()) return std::nullopt;

    nlohmann::json body;
    body["model"] = model_;
    body["max_tokens"] = max_tokens_;
    if (json_mode_)
        body["response_format"] = nlohmann::json::object({{"type", "json_object"}});

    nlohmann::json messages = nlohmann::json::array();
    if (!system_prompt.empty())
        messages.push_back({{"role", "system"}, {"content", system_prompt}});
    messages.push_back({{"role", "user"}, {"content", user_message}});
    body["messages"] = messages;

    std::string body_str = body.dump();
    std::string response_body;
    long http_code = 0;
    int retries = 3;

    for (int attempt = 1; attempt <= retries; ++attempt) {
        CURL* curl = curl_easy_init();
        if (!curl) return std::nullopt;

        std::string url = base_url_;
        if (url.empty()) url = "https://api.openai.com/v1/chat/completions";
        if (url.back() != '/') url += "/";
        if (url.find("/v1/") == std::string::npos) url += "v1/chat/completions";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + key).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return std::nullopt;
        if (http_code >= 200 && http_code < 300) {
            try {
                auto j = nlohmann::json::parse(response_body);
                if (j.contains("choices") && !j["choices"].empty()) {
                    auto& first = j["choices"][0];
                    if (first.contains("message") && first["message"].contains("content"))
                        return first["message"]["content"].get<std::string>();
                }
            } catch (...) {}
            return std::nullopt;
        }
        if (http_code != 429 && (http_code < 500 || http_code >= 600))
            return std::nullopt;
        if (attempt < retries) {
            int delay_ms = 1000 * (1 << attempt);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    return std::nullopt;
}

} // namespace scrapellm
