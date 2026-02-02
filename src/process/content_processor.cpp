// content_processor.cpp - Content processing pipeline
// LLM Documentation Scraper - C++ Implementation

#include "process/content_processor.hpp"
#include "parse/normalizer.hpp"
#include "utils/sanitize.hpp"
#include <fstream>

namespace docscraper::process {

ContentProcessor::ContentProcessor(ImageProcessor& img_processor, const crawler::AppConfig& app_config)
    : img_processor_(img_processor), app_config_(app_config) {}

std::string ContentProcessor::get_output_path(
    const std::string& url,
    const std::string& site_output_dir,
    const std::string& allowed_domain
) const {
    return parse::url_to_filepath(url, allowed_domain, site_output_dir);
}

ProcessResult ContentProcessor::extract_process_and_save(
    const parse::HTMLDocument& doc,
    const std::string& final_url,
    const crawler::SiteConfig& site_config,
    const std::string& site_output_dir
) {
    ProcessResult result;

    // Detect framework / select content
    auto detection = detector_.detect(doc, final_url);
    auto selector = site_config.content_selector;
    if (detect::is_auto_selector(selector)) {
        selector = detection.selector;
    }

    std::optional<parse::HTMLElement> content_elem = doc.select_first(selector);
    std::string content_text;

    if (content_elem.has_value()) {
        content_text = content_elem->text();
    } else {
        // Fallback to readability
        auto read = readability_.extract(doc);
        if (!read.success) {
            result.success = false;
            result.error = "Content selector not found and readability failed";
            return result;
        }
        content_text = read.content;
    }

    // Convert to Markdown (simple)
    result.markdown = markdown_converter_.convert(content_text);
    result.token_count = count_tokens(result.markdown);

    // Extract links
    result.extracted_links = link_processor_.extract_links(doc);

    // Process images (no-op download)
    if (content_elem.has_value()) {
        auto images = img_processor_.process_images(*content_elem, final_url, site_config, site_output_dir);
        result.image_count = static_cast<int>(images.size());
    }

    // Save markdown
    result.saved_file_path = get_output_path(final_url, site_output_dir, site_config.allowed_domain);
    std::string dir = docscraper::utils::parent_directory(result.saved_file_path);
    if (!dir.empty()) {
        docscraper::utils::ensure_directory(dir);
    }

    std::ofstream out(result.saved_file_path);
    if (!out.is_open()) {
        result.success = false;
        result.error = "Failed to write output file";
        return result;
    }
    out << result.markdown;

    result.success = true;
    return result;
}

} // namespace docscraper::process
