// image_processor.hpp - Image processing
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "parse/html_parser.hpp"
#include "crawler/config.hpp"
#include "storage/store_interface.hpp"
#include <map>
#include <string>

namespace docscraper::process {

struct ImageData {
    std::string original_url;
    std::string local_path;
    std::string caption;
};

class ImageProcessor {
public:
    ImageProcessor(const crawler::AppConfig& app_config, storage::ImageStore& store);

    std::map<std::string, ImageData> process_images(
        parse::HTMLElement& content,
        const std::string& page_url,
        const crawler::SiteConfig& site_config,
        const std::string& site_output_dir
    );

private:
    crawler::AppConfig app_config_;
    storage::ImageStore& store_;

    std::string compute_local_path(
        const std::string& img_url,
        const std::string& site_output_dir
    );
};

} // namespace docscraper::process
