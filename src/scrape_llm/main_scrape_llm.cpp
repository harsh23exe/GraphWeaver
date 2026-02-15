#include "scrape_llm/cli_config.hpp"
#include "scrape_llm/pipeline.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    scrapellm::RunConfig config;
    if (!scrapellm::parse_cli(argc, argv, config)) {
        return 0;
    }

    auto logger = spdlog::stdout_color_mt("scrape-llm");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(logger);

    std::error_code ec;
    std::filesystem::create_directories(config.out_dir, ec);
    if (ec) {
        spdlog::error("Could not create output directory: {}", config.out_dir);
        return 1;
    }

    return scrapellm::run_pipeline(config);
}
