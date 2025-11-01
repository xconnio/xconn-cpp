#pragma once

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/types.hpp"

extern "C" {
typedef struct Serializer Serializer;
}

namespace xconn {

class BaseSession;

constexpr std::size_t MAX_MSG_SIZE = (1 << 24);

class SessionJoiner {
   public:
    SessionJoiner(Authenticator authenticator, SerializerType serializer_type);
    ~SessionJoiner();

    std::unique_ptr<BaseSession> join(std::string& uri, std::string& realm);

   private:
    Authenticator authenticator_;
    SerializerType serializer_type_;
    Serializer* serializer_;
};

}  // namespace xconn
