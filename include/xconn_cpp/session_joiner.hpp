#pragma once

#include "xconn_cpp/base_session.hpp"

#include "wampproto/authenticators/authenticator.h"
#include "wampproto/serializers/serializer.h"
#include "wampproto/transports/rawsocket.h"

constexpr std::size_t MAX_MSG_SIZE = (1 << 24);

class SessionJoiner {
   public:
    SessionJoiner(ClientAuthenticator *authenticator, SerializerType serializer);
    ~SessionJoiner();

    BaseSession join(std::string &uri, std::string &realm);

   private:
    ClientAuthenticator *authenticator_;
    SerializerType serializer_type_;
    Serializer *serializer_;
};
