#pragma once
#include "xconn_cpp/url_parser.hpp"

#include "transports/tcp_transport.hpp"
#include "transports/transport.hpp"
#include "transports/unix_transport.hpp"

inline std::unique_ptr<Transport> create_transport(asio::io_context& io, const UrlParser& url) {
    if (url.scheme == "tcp" || url.scheme == "rs") {
        return std::make_unique<TcpTransport>(io);
    } else if (url.scheme == "unix" || url.scheme == "unix+rs") {
        return std::make_unique<UnixTransport>(io);
    } else {
        throw std::invalid_argument("Unknown transport scheme: " + url.scheme);
    }
}
