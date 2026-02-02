// main.cpp - Doc Scraper CLI Entrypoint
// LLM Documentation Scraper - C++ Implementation

#include <iostream>
#include <string>
#include <filesystem>
#include <sstream>
#include <vector>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "crawler/config.hpp"
#include "orchestrate/orchestrator.hpp"

constexpr const char* VERSION = "1.0.0";

void setup_logging(const std::string& log_level) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/crawler.log", 1024 * 1024 * 10, 3);
    
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("crawler", sinks.begin(), sinks.end());
    
    if (log_level == "trace") {
        logger->set_level(spdlog::level::trace);
    } else if (log_level == "debug") {
        logger->set_level(spdlog::level::debug);
    } else if (log_level == "info") {
        logger->set_level(spdlog::level::info);
    } else if (log_level == "warn") {
        logger->set_level(spdlog::level::warn);
    } else if (log_level == "error") {
        logger->set_level(spdlog::level::err);
    } else {
        logger->set_level(spdlog::level::info);
    }
    
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    spdlog::set_default_logger(logger);
}

void print_usage() {
    std::cout << "doc-scraper - LLM Documentation Scraper\n\n";
    std::cout << "Commands:\n";
    std::cout << "  crawl       Start a fresh crawl\n";
    std::cout << "  resume      Resume an interrupted crawl\n";
    std::cout << "  validate    Validate configuration file\n";
    std::cout << "  list-sites  List configured sites\n";
    std::cout << "  version     Show version information\n";
    std::cout << "\nUse 'crawler <command> --help' for more information about a command.\n";
}

static std::vector<std::string> split_sites(const std::string& sites_str) {
    std::vector<std::string> sites;
    std::istringstream iss(sites_str);
    std::string item;
    while (std::getline(iss, item, ',')) {
        if (!item.empty()) {
            sites.push_back(item);
        }
    }
    return sites;
}

int cmd_crawl(int argc, char** argv) {
    cxxopts::Options options("crawl", "Start a fresh documentation crawl");
    options.add_options()
        ("c,config", "Path to config file", cxxopts::value<std::string>()->default_value("config.yaml"))
        ("s,site", "Site key from config", cxxopts::value<std::string>())
        ("sites", "Comma-separated site keys", cxxopts::value<std::string>())
        ("all-sites", "Crawl all configured sites")
        ("l,loglevel", "Log level (trace, debug, info, warn, error)", 
            cxxopts::value<std::string>()->default_value("info"))
        ("incremental", "Enable incremental crawling (skip unchanged pages)")
        ("llm-guide", "Enable LLM-guided URL discovery")
        ("h,help", "Print help");
    
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << "\n";
            return 0;
        }

        setup_logging(result["loglevel"].as<std::string>());

        std::string config_path = result["config"].as<std::string>();
        auto app_config = docscraper::crawler::AppConfig::load_from_file(config_path);

        std::vector<std::string> site_keys;
        if (result.count("all-sites")) {
            site_keys = app_config.get_site_keys();
        } else if (result.count("sites")) {
            site_keys = split_sites(result["sites"].as<std::string>());
        } else if (result.count("site")) {
            site_keys.push_back(result["site"].as<std::string>());
        } else {
            std::cerr << "Error: --site, --sites, or --all-sites required\n";
            return 1;
        }

        spdlog::info("Starting crawl for {} site(s)", site_keys.size());

        docscraper::orchestrate::Orchestrator orch(app_config, site_keys, false);
        auto results = orch.run();

        for (const auto& res : results) {
            if (res.success) {
                std::cout << "[OK] " << res.site_key << ": "
                          << res.pages_processed << " pages ("
                          << res.duration.count() << " ms)\n";
            } else {
                std::cerr << "[FAIL] " << res.site_key << ": " << res.error << "\n";
            }
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << "\n";
        return 1;
    }
}

int cmd_resume(int argc, char** argv) {
    cxxopts::Options options("resume", "Resume an interrupted crawl");
    options.add_options()
        ("c,config", "Path to config file", cxxopts::value<std::string>()->default_value("config.yaml"))
        ("s,site", "Site key from config", cxxopts::value<std::string>())
        ("l,loglevel", "Log level", cxxopts::value<std::string>()->default_value("info"))
        ("h,help", "Print help");
    
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << "\n";
            return 0;
        }

        setup_logging(result["loglevel"].as<std::string>());

        std::string config_path = result["config"].as<std::string>();
        auto app_config = docscraper::crawler::AppConfig::load_from_file(config_path);

        std::vector<std::string> site_keys;
        if (result.count("site")) {
            site_keys.push_back(result["site"].as<std::string>());
        } else {
            std::cerr << "Error: --site required for resume\n";
            return 1;
        }

        docscraper::orchestrate::Orchestrator orch(app_config, site_keys, true);
        auto results = orch.run();

        for (const auto& res : results) {
            if (res.success) {
                std::cout << "[OK] " << res.site_key << ": "
                          << res.pages_processed << " pages ("
                          << res.duration.count() << " ms)\n";
            } else {
                std::cerr << "[FAIL] " << res.site_key << ": " << res.error << "\n";
            }
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << "\n";
        return 1;
    }
}

int cmd_validate(int argc, char** argv) {
    cxxopts::Options options("validate", "Validate configuration file");
    options.add_options()
        ("c,config", "Path to config file", cxxopts::value<std::string>()->default_value("config.yaml"))
        ("h,help", "Print help");
    
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << "\n";
            return 0;
        }

        std::string config_path = result["config"].as<std::string>();
        auto app_config = docscraper::crawler::AppConfig::load_from_file(config_path);
        auto validation = app_config.validate();

        for (const auto& w : validation.warnings) {
            std::cout << "[WARN] " << w << "\n";
        }
        for (const auto& e : validation.errors) {
            std::cout << "[ERROR] " << e << "\n";
        }

        if (validation.valid) {
            std::cout << "Config is valid\n";
            return 0;
        }

        return 1;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << "\n";
        return 1;
    }
}

int cmd_list_sites(int argc, char** argv) {
    cxxopts::Options options("list-sites", "List configured sites");
    options.add_options()
        ("c,config", "Path to config file", cxxopts::value<std::string>()->default_value("config.yaml"))
        ("h,help", "Print help");
    
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << "\n";
            return 0;
        }

        std::string config_path = result["config"].as<std::string>();
        auto app_config = docscraper::crawler::AppConfig::load_from_file(config_path);

        auto keys = app_config.get_site_keys();
        for (const auto& k : keys) {
            std::cout << k << "\n";
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char** argv) {
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories("logs");
    
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "crawl") {
        return cmd_crawl(argc - 1, argv + 1);
    } else if (command == "resume") {
        return cmd_resume(argc - 1, argv + 1);
    } else if (command == "validate") {
        return cmd_validate(argc - 1, argv + 1);
    } else if (command == "list-sites") {
        return cmd_list_sites(argc - 1, argv + 1);
    } else if (command == "version" || command == "--version" || command == "-v") {
        std::cout << "doc-scraper v" << VERSION << "\n";
        std::cout << "LLM Documentation Scraper - C++ Implementation\n";
        return 0;
    } else if (command == "help" || command == "--help" || command == "-h") {
        print_usage();
        return 0;
    } else {
        std::cerr << "Unknown command: " << command << "\n\n";
        print_usage();
        return 1;
    }
}
