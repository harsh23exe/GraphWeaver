// sanitize.cpp - Filename and Path Sanitization Implementation
// LLM Documentation Scraper - C++ Implementation

#include "utils/sanitize.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <sstream>
#include <regex>

namespace docscraper::utils {

// Reserved filenames on Windows
static const std::set<std::string> RESERVED_NAMES = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
};

// Characters invalid in filenames
static const std::string INVALID_CHARS = "<>:\"/\\|?*\0";

std::string sanitize_filename(const std::string& input, size_t max_length) {
    if (input.empty()) {
        return "unnamed";
    }
    
    std::string result;
    result.reserve(input.size());
    
    for (char c : input) {
        // Check for invalid characters
        if (INVALID_CHARS.find(c) != std::string::npos) {
            result += '_';
        } else if (static_cast<unsigned char>(c) < 32) {
            // Control characters
            result += '_';
        } else {
            result += c;
        }
    }
    
    // Collapse multiple underscores
    result = collapse_duplicates(result, '_');
    
    // Remove leading/trailing dots and spaces
    while (!result.empty() && (result.front() == '.' || result.front() == ' ')) {
        result.erase(0, 1);
    }
    while (!result.empty() && (result.back() == '.' || result.back() == ' ')) {
        result.pop_back();
    }
    
    // Check for reserved names (case-insensitive)
    std::string upper = result;
    std::transform(upper.begin(), upper.end(), upper.begin(),
        [](unsigned char c) { return std::toupper(c); });
    
    // Check without extension
    auto dot_pos = upper.find('.');
    std::string name_part = (dot_pos != std::string::npos) ? upper.substr(0, dot_pos) : upper;
    
    if (RESERVED_NAMES.count(name_part)) {
        result = "_" + result;
    }
    
    // Truncate to max length (respecting UTF-8)
    if (result.length() > max_length) {
        result = truncate_utf8(result, max_length);
    }
    
    // Ensure not empty
    if (result.empty()) {
        result = "unnamed";
    }
    
    return result;
}

std::string sanitize_path(const std::string& input) {
    std::vector<std::string> components;
    std::istringstream iss(input);
    std::string component;
    
    // Preserve leading slash
    bool absolute = !input.empty() && (input[0] == '/' || input[0] == '\\');
    
    while (std::getline(iss, component, '/')) {
        if (component.empty() || component == ".") {
            continue;
        }
        if (component == "..") {
            if (!components.empty()) {
                components.pop_back();
            }
            continue;
        }
        components.push_back(sanitize_filename(component));
    }
    
    std::ostringstream oss;
    if (absolute) {
        oss << "/";
    }
    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) oss << "/";
        oss << components[i];
    }
    
    return oss.str();
}

std::string collapse_duplicates(const std::string& input, char c) {
    std::string result;
    result.reserve(input.size());
    
    bool last_was_char = false;
    for (char ch : input) {
        if (ch == c) {
            if (!last_was_char) {
                result += ch;
                last_was_char = true;
            }
        } else {
            result += ch;
            last_was_char = false;
        }
    }
    
    return result;
}

std::string truncate_utf8(const std::string& input, size_t max_bytes) {
    if (input.length() <= max_bytes) {
        return input;
    }
    
    // Find a valid UTF-8 boundary
    size_t pos = max_bytes;
    while (pos > 0 && (static_cast<unsigned char>(input[pos]) & 0xC0) == 0x80) {
        --pos;
    }
    
    return input.substr(0, pos);
}

std::string strip_html_tags(const std::string& html) {
    std::string result;
    result.reserve(html.size());
    
    bool in_tag = false;
    for (char c : html) {
        if (c == '<') {
            in_tag = true;
        } else if (c == '>') {
            in_tag = false;
        } else if (!in_tag) {
            result += c;
        }
    }
    
    return result;
}

std::string normalize_whitespace(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    
    bool last_was_space = true;  // Start true to trim leading
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_was_space) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += c;
            last_was_space = false;
        }
    }
    
    // Trim trailing
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

std::string escape_tsv(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    
    for (char c : input) {
        switch (c) {
            case '\t': result += "\\t"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\\': result += "\\\\"; break;
            default: result += c;
        }
    }
    
    return result;
}

std::string escape_yaml(const std::string& input) {
    // Check if quoting is needed
    bool needs_quoting = input.empty();
    for (char c : input) {
        if (c == ':' || c == '#' || c == '\n' || c == '\r' || c == '\t' ||
            c == '"' || c == '\'' || c == '[' || c == ']' || c == '{' || c == '}' ||
            c == ',' || c == '&' || c == '*' || c == '!' || c == '|' || c == '>' ||
            c == '%' || c == '@' || c == '`') {
            needs_quoting = true;
            break;
        }
    }
    
    if (!needs_quoting) {
        return input;
    }
    
    // Use double quotes and escape
    std::string result = "\"";
    for (char c : input) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    result += "\"";
    
    return result;
}

bool is_valid_utf8(const std::string& input) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(input.data());
    size_t len = input.size();
    
    for (size_t i = 0; i < len; ) {
        unsigned char c = bytes[i];
        size_t char_len;
        
        if ((c & 0x80) == 0) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        } else {
            return false;  // Invalid leading byte
        }
        
        if (i + char_len > len) {
            return false;  // Truncated sequence
        }
        
        // Check continuation bytes
        for (size_t j = 1; j < char_len; ++j) {
            if ((bytes[i + j] & 0xC0) != 0x80) {
                return false;
            }
        }
        
        i += char_len;
    }
    
    return true;
}

std::string fix_utf8(const std::string& input) {
    if (is_valid_utf8(input)) {
        return input;
    }
    
    std::string result;
    result.reserve(input.size());
    
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(input.data());
    size_t len = input.size();
    
    for (size_t i = 0; i < len; ) {
        unsigned char c = bytes[i];
        size_t char_len = 1;
        bool valid = true;
        
        if ((c & 0x80) == 0) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        } else {
            valid = false;
        }
        
        if (valid && i + char_len <= len) {
            for (size_t j = 1; j < char_len; ++j) {
                if ((bytes[i + j] & 0xC0) != 0x80) {
                    valid = false;
                    break;
                }
            }
        } else {
            valid = false;
        }
        
        if (valid) {
            result.append(input, i, char_len);
            i += char_len;
        } else {
            // Replace invalid byte with U+FFFD (replacement character)
            result += "\xEF\xBF\xBD";
            ++i;
        }
    }
    
    return result;
}

std::string slugify(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    
    bool last_was_dash = true;  // Start true to skip leading dashes
    
    for (unsigned char c : input) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
            last_was_dash = false;
        } else if (!last_was_dash) {
            result += '-';
            last_was_dash = true;
        }
    }
    
    // Remove trailing dash
    while (!result.empty() && result.back() == '-') {
        result.pop_back();
    }
    
    return result.empty() ? "untitled" : result;
}

std::pair<std::string, std::string> split_path(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return {"", path};
    }
    return {path.substr(0, pos), path.substr(pos + 1)};
}

std::string parent_directory(const std::string& path) {
    return split_path(path).first;
}

std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    
    bool a_ends_slash = (a.back() == '/' || a.back() == '\\');
    bool b_starts_slash = (b.front() == '/' || b.front() == '\\');
    
    if (a_ends_slash && b_starts_slash) {
        return a + b.substr(1);
    } else if (a_ends_slash || b_starts_slash) {
        return a + b;
    } else {
        return a + "/" + b;
    }
}

std::string join_path(const std::vector<std::string>& components) {
    if (components.empty()) return "";
    
    std::string result = components[0];
    for (size_t i = 1; i < components.size(); ++i) {
        result = join_path(result, components[i]);
    }
    return result;
}

bool ensure_directory(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::string relative_path(const std::string& from, const std::string& to) {
    try {
        return std::filesystem::relative(to, from).string();
    } catch (const std::filesystem::filesystem_error&) {
        return to;
    }
}

} // namespace docscraper::utils
