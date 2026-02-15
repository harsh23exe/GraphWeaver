#include "scrape_llm/ssrf_guard.hpp"
#include "parse/normalizer.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace scrapellm {

static std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool is_http_or_https(const std::string& url) {
    auto parsed = docscraper::parse::URLNormalizer::parse(url);
    if (!parsed) return false;
    std::string scheme = to_lower(parsed->scheme);
    return scheme == "http" || scheme == "https";
}

// Check if host is localhost or private IP range.
static bool is_private_host(const std::string& host) {
    std::string h = to_lower(host);
    if (h == "localhost" || h == "127.0.0.1" || h == "::1" || h == "[::1]") return true;
    if (h.find("127.") == 0) return true;  // 127.0.0.0/8

    // IPv4 private: 10.x, 172.16-31.x, 192.168.x
    std::istringstream iss(h);
    int a = 0, b = 0, c = 0, d = 0;
    char dot;
    if (iss >> a >> dot >> b >> dot >> c >> dot >> d) {
        if (a == 10) return true;
        if (a == 172 && b >= 16 && b <= 31) return true;
        if (a == 192 && b == 168) return true;
    }
    return false;
}

bool url_allowed_ssrf(const std::string& url, bool allow_private_network) {
    auto parsed = docscraper::parse::URLNormalizer::parse(url);
    if (!parsed) return false;
    std::string scheme = to_lower(parsed->scheme);
    if (scheme != "http" && scheme != "https") return false;
    if (allow_private_network) return true;
    return !is_private_host(parsed->host);
}

} // namespace scrapellm
