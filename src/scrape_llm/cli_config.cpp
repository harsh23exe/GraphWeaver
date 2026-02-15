#include "scrape_llm/cli_config.hpp"
#include <cxxopts.hpp>
#include <iostream>
#include <cmath>

namespace scrapellm {

std::chrono::milliseconds RunConfig::rate_limit_delay_ms() const {
    if (rate_limit <= 0.0) return std::chrono::milliseconds(1000);
    return std::chrono::milliseconds(static_cast<int>(1000.0 / rate_limit));
}

bool parse_cli(int argc, char** argv, RunConfig& out_config) {
    cxxopts::Options options("scrape-llm", "LLM-powered web scraper");
    options.add_options()
        ("url", "Starting URL", cxxopts::value<std::string>())
        ("schema", "Natural-language description of desired structured output", cxxopts::value<std::string>())
        ("out", "Output directory", cxxopts::value<std::string>())
        ("format", "Output format: jsonl, json, csv", cxxopts::value<std::string>()->default_value("jsonl"))
        ("max-pages", "Max pages to crawl", cxxopts::value<int>()->default_value("30"))
        ("max-depth", "Max BFS depth", cxxopts::value<int>()->default_value("2"))
        ("keep-pages", "Max pages to parse with LLM", cxxopts::value<int>()->default_value("10"))
        ("rate-limit", "Requests per second", cxxopts::value<double>()->default_value("1.0"))
        ("respect-robots", "Honor robots.txt", cxxopts::value<bool>()->default_value("true"))
        ("allow-private-network", "Allow localhost/private IPs", cxxopts::value<bool>()->default_value("false"))
        ("model", "LLM model name", cxxopts::value<std::string>()->default_value("gpt-4.1-mini"))
        ("base-url", "LLM API base URL", cxxopts::value<std::string>()->default_value(""))
        ("csv", "Also emit CSV if flat")
        ("dry-run", "Crawl and select only, no parsing")
        ("h,help", "Print help")
        ("version", "Print version");

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << "\n";
            return false;
        }
        if (result.count("version")) {
            std::cout << "scrape-llm 1.0.0\n";
            return false;
        }

        if (!result.count("url") || !result.count("schema") || !result.count("out")) {
            std::cerr << "Error: --url, --schema, and --out are required.\n";
            std::cerr << options.help() << "\n";
            return false;
        }

        out_config.url = result["url"].as<std::string>();
        out_config.schema = result["schema"].as<std::string>();
        out_config.out_dir = result["out"].as<std::string>();
        out_config.format = result["format"].as<std::string>();
        out_config.max_pages = result["max-pages"].as<int>();
        out_config.max_depth = result["max-depth"].as<int>();
        out_config.keep_pages = result["keep-pages"].as<int>();
        out_config.rate_limit = result["rate-limit"].as<double>();
        out_config.respect_robots = result["respect-robots"].as<bool>();
        out_config.allow_private_network = result["allow-private-network"].as<bool>();
        out_config.model = result["model"].as<std::string>();
        out_config.base_url = result["base-url"].as<std::string>();
        out_config.emit_csv = result.count("csv") > 0;
        out_config.dry_run = result.count("dry-run") > 0;

        if (out_config.max_pages < 1) out_config.max_pages = 30;
        if (out_config.max_depth < 0) out_config.max_depth = 2;
        if (out_config.keep_pages < 1) out_config.keep_pages = 10;
        if (out_config.rate_limit <= 0.0) out_config.rate_limit = 1.0;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
}

} // namespace scrapellm
