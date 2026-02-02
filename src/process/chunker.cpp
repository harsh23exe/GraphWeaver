// chunker.cpp - Markdown chunking
// LLM Documentation Scraper - C++ Implementation

#include "process/chunker.hpp"
#include "process/tokenizer.hpp"
#include <regex>

namespace docscraper::process {

Chunker::Chunker(const ChunkerConfig& config) : config_(config) {}

std::vector<std::string> Chunker::split_by_headings(const std::string& markdown) const {
    std::vector<std::string> sections;
    std::regex heading_regex(R"(^#{1,6}\s+.+$)", std::regex::multiline);

    std::sregex_iterator it(markdown.begin(), markdown.end(), heading_regex);
    std::sregex_iterator end;

    if (it == end) {
        sections.push_back(markdown);
        return sections;
    }

    size_t last_pos = 0;
    for (; it != end; ++it) {
        size_t pos = static_cast<size_t>(it->position());
        if (pos > last_pos) {
            sections.push_back(markdown.substr(last_pos, pos - last_pos));
        }
        last_pos = pos;
    }
    if (last_pos < markdown.size()) {
        sections.push_back(markdown.substr(last_pos));
    }
    return sections;
}

std::vector<std::string> Chunker::extract_heading_hierarchy(const std::string& section) const {
    std::vector<std::string> headings;
    std::regex heading_regex(R"(^#{1,6}\s+(.+)$)", std::regex::multiline);
    std::smatch match;
    if (std::regex_search(section, match, heading_regex)) {
        headings.push_back(match[1].str());
    }
    return headings;
}

std::vector<Chunk> Chunker::chunk_markdown(const std::string& markdown) const {
    std::vector<Chunk> chunks;
    auto sections = split_by_headings(markdown);

    for (const auto& section : sections) {
        int tokens = count_tokens(section);
        if (tokens <= config_.max_chunk_size) {
            Chunk c;
            c.content = section;
            c.heading_hierarchy = extract_heading_hierarchy(section);
            c.token_count = tokens;
            chunks.push_back(std::move(c));
        } else {
            // Simple fallback: split by max size
            size_t start = 0;
            while (start < section.size()) {
                size_t len = std::min(section.size() - start, static_cast<size_t>(config_.max_chunk_size * 4));
                Chunk c;
                c.content = section.substr(start, len);
                c.heading_hierarchy = extract_heading_hierarchy(section);
                c.token_count = count_tokens(c.content);
                chunks.push_back(std::move(c));
                if (len < config_.chunk_overlap * 4) break;
                start += len - static_cast<size_t>(config_.chunk_overlap * 4);
            }
        }
    }
    return chunks;
}

} // namespace docscraper::process
