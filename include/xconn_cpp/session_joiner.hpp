#pragma once

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/base_session.hpp"
#include "xconn_cpp/types.hpp"

#include "wampproto/serializers/serializer.h"

constexpr std::size_t MAX_MSG_SIZE = (1 << 24);

class SessionJoiner {
   public:
    SessionJoiner(xconn::Authenticator authenticator, xconn::SerializerType serializer_type);
    ~SessionJoiner();

    BaseSession join(std::string& uri, std::string& realm);

   private:
    xconn::Authenticator authenticator_;
    xconn::SerializerType serializer_type_;
    Serializer* serializer_;
};
