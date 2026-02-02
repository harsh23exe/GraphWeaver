// robots.cpp - robots.txt Parser Implementation
// LLM Documentation Scraper - C++ Implementation

#include "fetch/robots.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace docscraper::fetch {

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

static std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

void RobotsHandler::parse(const std::string& robots_txt) {
    rules_.clear();

    std::istringstream iss(robots_txt);
    std::string line;
    std::string current_agent;

    while (std::getline(iss, line)) {
        // Remove comments
        auto hash_pos = line.find('#');
        if (hash_pos != std::string::npos) {
            line = line.substr(0, hash_pos);
        }
        line = trim(line);
        if (line.empty()) continue;

        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = to_lower(trim(line.substr(0, colon_pos)));
        std::string value = trim(line.substr(colon_pos + 1));

        if (key == "user-agent") {
            current_agent = to_lower(value);
            if (rules_.find(current_agent) == rules_.end()) {
                RobotsRule rule;
                rule.user_agent = current_agent;
                rules_[current_agent] = rule;
            }
        } else if (key == "disallow" && !current_agent.empty()) {
            rules_[current_agent].disallow.push_back(value);
        } else if (key == "allow" && !current_agent.empty()) {
            rules_[current_agent].allow.push_back(value);
        } else if (key == "sitemap") {
            // Sitemaps are global
            if (!current_agent.empty()) {
                rules_[current_agent].sitemaps.push_back(value);
            } else {
                // Store under wildcard
                rules_["*"].sitemaps.push_back(value);
            }
        } else if (key == "crawl-delay" && !current_agent.empty()) {
            try {
                rules_[current_agent].crawl_delay_seconds = std::stoi(value);
            } catch (...) {
                // Ignore invalid crawl-delay
            }
        }
    }
}

const RobotsRule* RobotsHandler::get_rule_for_agent(const std::string& user_agent) const {
    std::string agent = to_lower(user_agent);

    auto it = rules_.find(agent);
    if (it != rules_.end()) {
        return &it->second;
    }

    // Fallback to wildcard
    it = rules_.find("*");
    if (it != rules_.end()) {
        return &it->second;
    }

    return nullptr;
}

bool RobotsHandler::matches_rule(const std::string& path, const std::string& rule_path) {
    if (rule_path.empty()) return false;
    if (rule_path == "/") return true;

    // Simple prefix match (RFC 9309 basic)
    if (path.rfind(rule_path, 0) == 0) {
        return true;
    }
    return false;
}

bool RobotsHandler::is_allowed(const std::string& url_path, const std::string& user_agent) const {
    const RobotsRule* rule = get_rule_for_agent(user_agent);
    if (!rule) {
        return true;  // No rules, allow
    }

    // If path is empty, allow
    if (url_path.empty()) return true;

    // Most specific rule wins: allow overrides disallow if longer match
    int best_allow = -1;
    int best_disallow = -1;

    for (const auto& allow : rule->allow) {
        if (matches_rule(url_path, allow)) {
            int len = static_cast<int>(allow.length());
            if (len > best_allow) best_allow = len;
        }
    }

    for (const auto& disallow : rule->disallow) {
        if (matches_rule(url_path, disallow)) {
            int len = static_cast<int>(disallow.length());
            if (len > best_disallow) best_disallow = len;
        }
    }

    if (best_allow == -1 && best_disallow == -1) {
        return true;
    }
    if (best_allow >= best_disallow) {
        return true;
    }
    return false;
}

std::vector<std::string> RobotsHandler::get_sitemaps() const {
    std::vector<std::string> sitemaps;
    for (const auto& [_, rule] : rules_) {
        for (const auto& s : rule.sitemaps) {
            sitemaps.push_back(s);
        }
    }
    // Deduplicate
    std::sort(sitemaps.begin(), sitemaps.end());
    sitemaps.erase(std::unique(sitemaps.begin(), sitemaps.end()), sitemaps.end());
    return sitemaps;
}

} // namespace docscraper::fetch
