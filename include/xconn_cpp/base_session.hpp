#pragma once

#include "xconn_cpp/socket_transport.hpp"

#include "wampproto/serializers/serializer.h"
#include "wampproto/session_details.h"

class BaseSession {
   public:
    BaseSession(std::shared_ptr<SocketTransport> transport, SessionDetails* session_details, Serializer* serializer);

    std::shared_ptr<SocketTransport> transport() const;
    uint64_t id() const;
    const char* realm() const;
    const char* authid() const;
    const char* authrole() const;
    SessionDetails* session_details() const;

    // Core methods
    void send(Bytes& bytes);
    Bytes receive();

    void send_message(const Message* msg);
    Message* receive_message();

    void close();

   private:
    std::shared_ptr<SocketTransport> transport_;
    SessionDetails* session_details_;
    Serializer* serializer_;
};
