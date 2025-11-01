#include "xconn_cpp/url_parser.hpp"

#include <algorithm>

namespace xconn {

UrlParser parse_url(const std::string& url) {
    UrlParser parts;

    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return parts;

    parts.scheme = url.substr(0, scheme_end);
    std::string rest = url.substr(scheme_end + 3);

    if (parts.scheme.rfind("unix", 0) == 0) {
        parts.host = rest;
        parts.port = "";
        return parts;
    }

    size_t colon_pos = rest.find(':');
    size_t slash_pos = rest.find('/');

    if (colon_pos != std::string::npos && (slash_pos == std::string::npos || colon_pos < slash_pos)) {
        parts.host = rest.substr(0, colon_pos);
        parts.port = rest.substr(colon_pos + 1, slash_pos - colon_pos - 1);
    } else {
        parts.host = rest.substr(0, slash_pos);
        parts.port = "";
    }

    if (slash_pos != std::string::npos) parts.host = rest.substr(0, std::min(colon_pos, slash_pos));

    return parts;
}

}  // namespace xconn
