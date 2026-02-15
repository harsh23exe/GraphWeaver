#include "scrape_llm/content_extractor.hpp"
#include "parse/html_parser.hpp"
#include "parse/normalizer.hpp"
#include <sstream>
#include <algorithm>

namespace scrapellm {

static std::string trim_ws(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

ExtractedContent extract_content(const docscraper::parse::HTMLDocument& doc, const std::string& page_url) {
    ExtractedContent out;
    out.url = page_url;
    out.main_text = doc.main_text();

    auto title_el = doc.select_first("title");
    if (title_el) out.title = trim_ws(title_el->text());

    auto metas = doc.select("meta");
    for (const auto& m : metas) {
        std::string name = m.attr("name");
        if (name == "description") {
            out.meta_description = m.attr("content");
            break;
        }
    }

    for (const std::string& tag : {"h1", "h2", "h3"}) {
        auto els = doc.select(tag);
        for (const auto& el : els) {
            std::string t = trim_ws(el.text());
            if (!t.empty()) out.headings.push_back(t);
        }
    }

    auto tables = doc.select("table");
    for (const auto& table : tables) {
        std::ostringstream tsv;
        auto rows = table.select("tr");
        for (size_t i = 0; i < rows.size(); ++i) {
            auto cells = rows[i].select("td, th");
            for (size_t j = 0; j < cells.size(); ++j) {
                if (j) tsv << "\t";
                std::string cell_text = trim_ws(cells[j].text());
                for (char c : cell_text) {
                    if (c == '\t' || c == '\n' || c == '\r') tsv << ' ';
                    else tsv << c;
                }
            }
            tsv << "\n";
        }
        std::string table_str = tsv.str();
        if (!table_str.empty() && table_str != "\n")
            out.tables_tsv.push_back(table_str);
    }

    return out;
}

PageDigest make_digest(const ExtractedContent& content, size_t max_preview_chars) {
    PageDigest d;
    d.url = content.url;
    d.title = content.title;
    d.headings = content.headings;
    d.text_preview = content.main_text.substr(0, max_preview_chars);
    if (content.main_text.size() > max_preview_chars)
        d.text_preview += "...";
    return d;
}

std::vector<std::string> extract_links_absolute(
    const docscraper::parse::HTMLDocument& doc,
    const std::string& base_url
) {
    std::vector<std::string> out;
    auto anchors = doc.select("a");
    for (const auto& a : anchors) {
        std::string href = a.attr("href");
        if (href.empty()) continue;
        auto resolved = docscraper::parse::URLNormalizer::resolve(base_url, href);
        if (resolved && docscraper::parse::URLNormalizer::is_valid_http_url(*resolved))
            out.push_back(*resolved);
    }
    return out;
}

} // namespace scrapellm
