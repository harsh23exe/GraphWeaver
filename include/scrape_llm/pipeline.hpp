#pragma once

#include "scrape_llm/cli_config.hpp"
#include "scrape_llm/types.hpp"
#include "scrape_llm/llm_client.hpp"
#include <memory>
#include <vector>

namespace scrapellm {

// Full pipeline: schema infer -> crawl -> relevance -> extract -> parse -> validate -> dedupe -> output + report.
int run_pipeline(const RunConfig& config);

} // namespace scrapellm
