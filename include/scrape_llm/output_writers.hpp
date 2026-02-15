#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace scrapellm {

// Write records to out_dir: records.jsonl (always), optionally records.json and records.csv.
void write_outputs(
    const std::string& out_dir,
    const std::string& format,
    bool emit_csv,
    const std::vector<nlohmann::json>& records
);

// Write schema to out_dir/schema.json.
void write_schema(const std::string& out_dir, const nlohmann::json& schema_obj);

} // namespace scrapellm
