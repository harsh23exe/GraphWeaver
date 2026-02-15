#pragma once

#include "scrape_llm/types.hpp"
#include <string>

namespace scrapellm {

// Write report.json and report.md under out_dir.
void write_report(const std::string& out_dir, const RunReport& report);

} // namespace scrapellm
