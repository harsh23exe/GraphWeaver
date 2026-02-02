// robots.hpp - robots.txt Parser
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <vector>
#include <map>

namespace docscraper::fetch {

struct RobotsRule {
    std::string user_agent;
    std::vector<std::string> disallow;
    std::vector<std::string> allow;
    std::vector<std::string> sitemaps;
    int crawl_delay_seconds = -1;
};

class RobotsHandler {
public:
    RobotsHandler() = default;

    // Parse robots.txt content (no network)
    void parse(const std::string& robots_txt);

    // Check if URL path is allowed for user agent
    bool is_allowed(const std::string& url_path, const std::string& user_agent = "*") const;

    // Get sitemap URLs discovered in robots.txt
    std::vector<std::string> get_sitemaps() const;

private:
    std::map<std::string, RobotsRule> rules_;

    const RobotsRule* get_rule_for_agent(const std::string& user_agent) const;
    static bool matches_rule(const std::string& path, const std::string& rule_path);
};

} // namespace docscraper::fetch
