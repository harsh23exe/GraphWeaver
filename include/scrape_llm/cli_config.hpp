#pragma once

#include <string>
#include <chrono>
#include <cstddef>

namespace scrapellm {

struct RunConfig {
    std::string url;
    std::string schema;
    std::string out_dir;
    std::string format = "jsonl";  // jsonl | json | csv
    int max_pages = 30;
    int max_depth = 2;
    int keep_pages = 10;
    double rate_limit = 1.0;       // requests per second
    bool respect_robots = true;
    bool allow_private_network = false;
    std::string model = "gpt-4.1-mini";
    std::string base_url;          // empty = use default Gemini/OpenAI
    bool emit_csv = false;
    bool dry_run = false;

    std::chrono::milliseconds rate_limit_delay_ms() const;
};

// Parse argv into RunConfig. Returns false on parse error or --help/--version.
bool parse_cli(int argc, char** argv, RunConfig& out_config);

} // namespace scrapellm
