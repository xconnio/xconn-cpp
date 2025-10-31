#pragma once

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/types.hpp"

class BaseSession;

extern "C" {
typedef struct Serializer Serializer;
}

constexpr std::size_t MAX_MSG_SIZE = (1 << 24);

class SessionJoiner {
   public:
    SessionJoiner(xconn::Authenticator authenticator, xconn::SerializerType_ serializer_type);
    ~SessionJoiner();

    std::unique_ptr<BaseSession> join(std::string& uri, std::string& realm);

   private:
    xconn::Authenticator authenticator_;
    xconn::SerializerType_ serializer_type_;
    Serializer* serializer_;
};
