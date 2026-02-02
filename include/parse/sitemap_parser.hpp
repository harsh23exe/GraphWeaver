// sitemap_parser.hpp - Sitemap XML parsing
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <vector>

namespace docscraper::parse {

struct SitemapURL {
    std::string loc;
    std::string lastmod;
};

struct Sitemap {
    std::string loc;
    std::string lastmod;
};

class SitemapParser {
public:
    enum class SitemapType { URLSet, Index, Unknown };

    std::vector<SitemapURL> parse_urlset(const std::string& xml) const;
    std::vector<Sitemap> parse_index(const std::string& xml) const;
    SitemapType detect_type(const std::string& xml) const;
};

} // namespace docscraper::parse
