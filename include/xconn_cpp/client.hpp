#pragma once

#include <memory>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/session.hpp"
#include "xconn_cpp/types.hpp"

namespace xconn {

class Client {
   public:
    Authenticator authenticator;
    SerializerType serializer_type;

    Client(Authenticator authenticator, SerializerType serializer_type)
        : authenticator(std::move(authenticator)), serializer_type(serializer_type) {}

    ~Client();

    std::unique_ptr<Session> connect(std::string uri, std::string realm);
};

}  // namespace xconn
