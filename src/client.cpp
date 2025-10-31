#include "xconn_cpp/client.hpp"

#include <iostream>
#include <memory>

#include "xconn_cpp/internal/base_session.hpp"
#include "xconn_cpp/session.hpp"
#include "xconn_cpp/session_joiner.hpp"

namespace xconn {

Client::~Client() {}

std::unique_ptr<Session> Client::connect(std::string uri, std::string realm) {
    auto joiner = std::make_unique<SessionJoiner>(authenticator, serializer_type);
    auto base_session = joiner->join(uri, realm);
    auto session = std::make_unique<Session>(std::move(base_session));

    return session;
}

}  // namespace xconn
