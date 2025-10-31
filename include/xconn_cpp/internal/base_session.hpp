#pragma once

#include "xconn_cpp/internal/socket_transport.hpp"

class BaseSession {
   public:
    BaseSession(std::shared_ptr<SocketTransport> transport, const SessionDetails* session_details,
                Serializer* serializer);
    Serializer* serializer;

    std::shared_ptr<SocketTransport> transport() const;
    uint64_t id() const;
    const char* realm() const;
    const char* authid() const;
    const char* authrole() const;

    // Core methods
    void send(Bytes& bytes);
    Bytes receive();

    void send_message(const Message* msg);
    Message* receive_message();

    void close();

   private:
    std::shared_ptr<SocketTransport> transport_;
    const SessionDetails* session_details_;
};
