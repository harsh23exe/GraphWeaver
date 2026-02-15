#include "scrape_llm/output_writers.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace scrapellm {

namespace fs = std::filesystem;

void write_schema(const std::string& out_dir, const nlohmann::json& schema_obj) {
    fs::create_directories(out_dir);
    std::ofstream f(out_dir + "/schema.json");
    if (f) f << schema_obj.dump(2);
}

static bool is_flat_record(const nlohmann::json& j) {
    if (!j.is_object()) return false;
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.value().is_object() || it.value().is_array()) return false;
    }
    return true;
}

void write_outputs(const std::string& out_dir, const std::string& format, bool emit_csv,
                  const std::vector<nlohmann::json>& records) {
    fs::create_directories(out_dir);
    std::string path_base = out_dir + "/records";

    if (format == "jsonl" || format.empty()) {
        std::ofstream f(path_base + ".jsonl");
        for (const auto& r : records)
            f << r.dump() << "\n";
    }
    if (format == "json") {
        std::ofstream f(path_base + ".json");
        f << nlohmann::json(records).dump(2);
    }
    if (emit_csv) {
        bool flat = !records.empty();
        for (const auto& r : records) {
            if (!is_flat_record(r)) { flat = false; break; }
        }
        if (flat) {
            std::ofstream f(path_base + ".csv");
            std::vector<std::string> keys;
            for (const auto& r : records) {
                for (auto it = r.begin(); it != r.end(); ++it) {
                    if (std::find(keys.begin(), keys.end(), it.key()) == keys.end())
                        keys.push_back(it.key());
                }
            }
            std::sort(keys.begin(), keys.end());
            for (size_t i = 0; i < keys.size(); ++i)
                f << (i ? "," : "") << "\"" << keys[i] << "\"";
            f << "\n";
            for (const auto& r : records) {
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (i) f << ",";
                    std::string v;
                    if (r.contains(keys[i])) {
                        if (r[keys[i]].is_string()) v = r[keys[i]].get<std::string>();
                        else v = r[keys[i]].dump();
                    }
                    if (v.find('"') != std::string::npos || v.find(',') != std::string::npos || v.find('\n') != std::string::npos) {
                        f << '"';
                        for (char c : v) { if (c == '"') f << "\"\""; else f << c; }
                        f << '"';
                    } else f << '"' << v << '"';
                }
                f << "\n";
            }
        }
    }
}

} // namespace scrapellm
