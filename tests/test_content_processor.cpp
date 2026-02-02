// test_content_processor.cpp - Unit tests for content processor
#include <gtest/gtest.h>
#include "process/content_processor.hpp"
#include "storage/store_interface.hpp"
#include <filesystem>

using namespace docscraper::process;
using namespace docscraper::crawler;
using namespace docscraper::storage;

class DummyImageStore : public ImageStore {
public:
    std::pair<docscraper::models::ImageStatus, std::optional<docscraper::models::ImageDBEntry>>
    check_image_status(const std::string&) override {
        return {docscraper::models::ImageStatus::Unknown, std::nullopt};
    }

    void update_image_status(
        const std::string&,
        const docscraper::models::ImageDBEntry&
    ) override {}
};

TEST(ContentProcessorTest, ExtractAndSave) {
    std::string html = R"(
<html><body>
<main>
  <h1>Title</h1>
  <p>Hello world</p>
</main>
</body></html>
)";

    docscraper::parse::HTMLDocument doc(html);
    AppConfig app_config;
    DummyImageStore store;
    ImageProcessor img_processor(app_config, store);
    ContentProcessor processor(img_processor, app_config);

    SiteConfig site_config;
    site_config.allowed_domain = "example.com";
    site_config.content_selector = "main";

    std::string output_dir = "./test_output";
    std::filesystem::remove_all(output_dir);

    auto result = processor.extract_process_and_save(
        doc,
        "https://example.com/docs",
        site_config,
        output_dir
    );

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.saved_file_path.empty());
    EXPECT_TRUE(std::filesystem::exists(result.saved_file_path));

    std::filesystem::remove_all(output_dir);
}
