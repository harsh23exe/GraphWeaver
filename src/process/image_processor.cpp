// image_processor.cpp - Image processing
// LLM Documentation Scraper - C++ Implementation

#include "process/image_processor.hpp"
#include "parse/normalizer.hpp"
#include "utils/hash.hpp"
#include "utils/sanitize.hpp"

namespace docscraper::process {

ImageProcessor::ImageProcessor(const crawler::AppConfig& app_config, storage::ImageStore& store)
    : app_config_(app_config), store_(store) {}

std::string ImageProcessor::compute_local_path(
    const std::string& img_url,
    const std::string& site_output_dir
) {
    std::string hash = docscraper::utils::url_hash(img_url);
    std::string filename = "img_" + hash + ".bin";
    return docscraper::utils::join_path(site_output_dir, "images/" + filename);
}

std::map<std::string, ImageData> ImageProcessor::process_images(
    parse::HTMLElement& content,
    const std::string& /*page_url*/,
    const crawler::SiteConfig& site_config,
    const std::string& site_output_dir
) {
    std::map<std::string, ImageData> images;

    if (site_config.skip_images) {
        return images;
    }

    auto img_nodes = content.select("img");
    for (auto& img : img_nodes) {
        std::string src = img.attr("src");
        if (src.empty()) continue;

        ImageData data;
        data.original_url = src;
        data.local_path = compute_local_path(src, site_output_dir);
        data.caption = img.attr("alt");

        images[src] = data;

        // Store image entry as pending (actual download handled later)
        models::ImageDBEntry entry;
        entry.original_url = src;
        entry.local_path = data.local_path;
        entry.caption = data.caption;
        entry.status = models::ImageStatus::Pending;
        store_.update_image_status(src, entry);
    }

    return images;
}

} // namespace docscraper::process
