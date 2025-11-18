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

inline std::unique_ptr<Session> connectAnonymous(std::string uri, std::string realm, std::string auth_id = "") {
    auto authenticator = std::make_unique<AnonymousAuthenticator>(auth_id);
    auto client = Client(*authenticator, SerializerType::CBOR);

    return client.connect(uri, realm);
}

inline std::unique_ptr<Session> connectTicket(std::string uri, std::string realm, std::string auth_id,
                                              std::string ticket) {
    auto authenticator = std::make_unique<TicketAuthenticator>(auth_id, ticket);
    auto client = Client(*authenticator, SerializerType::CBOR);

    return client.connect(uri, realm);
}

inline std::unique_ptr<Session> connectWAMPCRA(std::string uri, std::string realm, std::string auth_id,
                                               std::string secret) {
    auto authenticator = std::make_unique<WAMPCRAAuthenticator>(auth_id, secret);
    auto client = Client(*authenticator, SerializerType::CBOR);

    return client.connect(uri, realm);
}

inline std::unique_ptr<Session> connectCryptosign(std::string uri, std::string realm, std::string auth_id,
                                                  std::string private_key_hex) {
    auto authenticator = std::make_unique<CryptosignAuthenticator>(auth_id, private_key_hex);
    auto client = Client(*authenticator, SerializerType::CBOR);

    return client.connect(uri, realm);
}

}  // namespace xconn
