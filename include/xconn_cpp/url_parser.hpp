#pragma once

#include <string>

namespace xconn {

struct UrlParser {
    std::string scheme;
    std::string host;
    std::string port;
};

UrlParser parse_url(const std::string& url);

}  // namespace xconn
