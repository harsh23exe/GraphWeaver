#include "scrape_llm/relevance_router.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace scrapellm {

static std::string digest_to_string(const PageDigest& d) {
    nlohmann::json j;
    j["url"] = d.url;
    j["title"] = d.title;
    j["headings"] = d.headings;
    j["text_preview"] = d.text_preview;
    return j.dump(2);
}

static std::string relevance_prompt(const std::string& user_schema, const std::string& digest_str) {
    return "You are a relevance filter for a web scraper. Given the extraction goal and a short digest of a page, decide if the page should be kept for extraction (KEEP) or skipped (SKIP).\n\n"
           "Extraction goal:\n" + user_schema + "\n\n"
           "Page digest:\n" + digest_str + "\n\n"
           "Respond with a single JSON object only (no markdown, no explanation):\n"
           "{ \"decision\": \"KEEP\" or \"SKIP\", \"reason\": \"brief reason\" }";
}

RelevanceDecision relevance_decide(ILlmClient& client, const std::string& user_schema, const PageDigest& digest) {
    RelevanceDecision out;
    std::string digest_str = digest_to_string(digest);
    auto resp = client.chat(relevance_prompt(user_schema, digest_str), "");
    if (!resp) {
        out.keep = false;
        out.reason = "LLM call failed";
        return out;
    }
    std::string raw = *resp;
    if (raw.size() >= 3 && raw.substr(0, 3) == "```") {
        size_t end = raw.find('\n');
        if (end != std::string::npos) raw = raw.substr(end + 1);
        size_t close = raw.find("```");
        if (close != std::string::npos) raw = raw.substr(0, close);
    }
    try {
        auto j = nlohmann::json::parse(raw);
        std::string dec = j.value("decision", "SKIP");
        out.reason = j.value("reason", "");
        out.keep = (dec == "KEEP" || dec == "Keep" || dec == "keep");
        return out;
    } catch (...) {
        out.keep = false;
        out.reason = "Parse error";
        return out;
    }
}

std::vector<PageDigest> select_pages_to_parse(ILlmClient& client, const std::string& user_schema,
                                               std::vector<PageDigest> digests, int keep_n) {
    std::vector<std::pair<PageDigest, bool>> scored;
    for (auto& d : digests) {
        RelevanceDecision dec = relevance_decide(client, user_schema, d);
        scored.emplace_back(std::move(d), dec.keep);
    }
    std::vector<PageDigest> out;
    for (auto& p : scored) {
        if (p.second) out.push_back(std::move(p.first));
        if (static_cast<int>(out.size()) >= keep_n) break;
    }
    out.resize(std::min(static_cast<int>(out.size()), keep_n));
    return out;
}

} // namespace scrapellm
