// chunker.hpp - Heading-aware Markdown chunking
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>
#include <vector>

namespace docscraper::process {

struct Chunk {
    std::string content;
    std::vector<std::string> heading_hierarchy;
    int token_count = 0;
};

struct ChunkerConfig {
    int max_chunk_size = 512;
    int chunk_overlap = 50;
};

class Chunker {
public:
    explicit Chunker(const ChunkerConfig& config);
    std::vector<Chunk> chunk_markdown(const std::string& markdown) const;

private:
    ChunkerConfig config_;
    std::vector<std::string> split_by_headings(const std::string& markdown) const;
    std::vector<std::string> extract_heading_hierarchy(const std::string& section) const;
};

} // namespace docscraper::process
