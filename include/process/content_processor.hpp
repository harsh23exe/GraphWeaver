// content_processor.hpp - Content processing pipeline
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "process/image_processor.hpp"
#include "process/link_processor.hpp"
#include "process/markdown_converter.hpp"
#include "process/tokenizer.hpp"
#include "detect/detector.hpp"
#include "detect/readability.hpp"
#include "crawler/config.hpp"
#include "models/models.hpp"
#include "parse/html_parser.hpp"

namespace docscraper::process {

struct ProcessResult {
    std::string title;
    std::string markdown;
    std::string saved_file_path;
    std::vector<std::string> extracted_links;
    int image_count = 0;
    int token_count = 0;
    bool success = false;
    std::string error;
};

class ContentProcessor {
public:
    ContentProcessor(ImageProcessor& img_processor, const crawler::AppConfig& app_config);

    ProcessResult extract_process_and_save(
        const parse::HTMLDocument& doc,
        const std::string& final_url,
        const crawler::SiteConfig& site_config,
        const std::string& site_output_dir
    );

private:
    ImageProcessor& img_processor_;
    crawler::AppConfig app_config_;
    detect::ContentDetector detector_;
    detect::ReadabilityExtractor readability_;
    LinkProcessor link_processor_;
    MarkdownConverter markdown_converter_;

    std::string get_output_path(
        const std::string& url,
        const std::string& site_output_dir,
        const std::string& allowed_domain
    ) const;
};

} // namespace docscraper::process
