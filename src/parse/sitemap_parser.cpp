// sitemap_parser.cpp - Sitemap XML parsing
// LLM Documentation Scraper - C++ Implementation

#include "parse/sitemap_parser.hpp"
#include <pugixml.hpp>

namespace docscraper::parse {

SitemapParser::SitemapType SitemapParser::detect_type(const std::string& xml) const {
    pugi::xml_document doc;
    if (!doc.load_string(xml.c_str())) {
        return SitemapType::Unknown;
    }
    if (doc.child("urlset")) return SitemapType::URLSet;
    if (doc.child("sitemapindex")) return SitemapType::Index;
    return SitemapType::Unknown;
}

std::vector<SitemapURL> SitemapParser::parse_urlset(const std::string& xml) const {
    std::vector<SitemapURL> urls;
    pugi::xml_document doc;
    if (!doc.load_string(xml.c_str())) return urls;

    auto urlset = doc.child("urlset");
    for (auto url_node : urlset.children("url")) {
        SitemapURL entry;
        entry.loc = url_node.child("loc").text().as_string();
        entry.lastmod = url_node.child("lastmod").text().as_string();
        if (!entry.loc.empty()) urls.push_back(entry);
    }
    return urls;
}

std::vector<Sitemap> SitemapParser::parse_index(const std::string& xml) const {
    std::vector<Sitemap> sitemaps;
    pugi::xml_document doc;
    if (!doc.load_string(xml.c_str())) return sitemaps;

    auto index = doc.child("sitemapindex");
    for (auto sm_node : index.children("sitemap")) {
        Sitemap entry;
        entry.loc = sm_node.child("loc").text().as_string();
        entry.lastmod = sm_node.child("lastmod").text().as_string();
        if (!entry.loc.empty()) sitemaps.push_back(entry);
    }
    return sitemaps;
}

} // namespace docscraper::parse
