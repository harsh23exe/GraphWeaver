#include "scrape_llm/report_generator.hpp"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace scrapellm {

namespace fs = std::filesystem;

void write_report(const std::string& out_dir, const RunReport& report) {
    fs::create_directories(out_dir);

    nlohmann::json j;
    j["pages_crawled"] = report.pages_crawled;
    j["pages_kept"] = report.pages_kept;
    j["records_emitted"] = report.records_emitted;
    j["validation_failures"] = report.validation_failures;
    j["repair_attempts"] = report.repair_attempts;
    j["repair_successes"] = report.repair_successes;
    j["tokens_estimate"] = report.tokens_estimate;
    j["crawl_ms"] = report.crawl_ms.count();
    j["llm_ms"] = report.llm_ms.count();
    j["errors"] = report.errors;
    j["pages_visited"] = report.pages_visited;

    std::ofstream f(out_dir + "/report.json");
    if (f) f << j.dump(2);

    std::ofstream md(out_dir + "/report.md");
    if (md) {
        md << "# Run Report\n\n";
        md << "- Pages crawled: " << report.pages_crawled << "\n";
        md << "- Pages kept: " << report.pages_kept << "\n";
        md << "- Records emitted: " << report.records_emitted << "\n";
        md << "- Validation failures: " << report.validation_failures << "\n";
        md << "- Repair attempts: " << report.repair_attempts << "\n";
        md << "- Repair successes: " << report.repair_successes << "\n";
        md << "- Tokens estimate: " << report.tokens_estimate << "\n";
        md << "- Crawl time (ms): " << report.crawl_ms.count() << "\n";
        md << "- LLM time (ms): " << report.llm_ms.count() << "\n";
        if (!report.errors.empty()) {
            md << "\n## Errors\n\n";
            for (const auto& e : report.errors) md << "- " << e << "\n";
        }
        if (!report.pages_visited.empty()) {
            md << "\n## Pages visited\n\n";
            for (const auto& p : report.pages_visited) md << "- " << p << "\n";
        }
    }
}

} // namespace scrapellm
