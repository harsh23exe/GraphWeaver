#pragma once

#include <string>

namespace scrapellm {

// Returns true if the URL is allowed (http/https and not private/local when allow_private is false).
bool url_allowed_ssrf(const std::string& url, bool allow_private_network);

// Returns true if scheme is http or https.
bool is_http_or_https(const std::string& url);

} // namespace scrapellm
