#pragma once

#include <string>

#include "xconn_cpp/types.hpp"

extern "C" {
typedef struct ClientAuthenticator ClientAuthenticator;
}

namespace xconn {

class Authenticator {
   public:
    virtual ~Authenticator() = default;

    Authenticator(std::string auth_id, std::string auth_data, Dict auth_extra);

    std::string auth_id;
    Dict auth_extra;
    std::string auth_method;
    std::string auth_data;

    ClientAuthenticator* authenticator;
};

class AnonymousAuthenticator : public Authenticator {
   public:
    AnonymousAuthenticator(std::string auth_id, Dict auth_extra);
};

class TicketAuthenticator : public Authenticator {
   public:
    TicketAuthenticator(std::string auth_id, std::string ticket, Dict auth_extra);
};

class CryptosignAuthenticator : public Authenticator {
   public:
    CryptosignAuthenticator(std::string auth_id, std::string private_key_hex, Dict auth_extra);
};

class WAMPCRAAuthenticator : public Authenticator {
   public:
    WAMPCRAAuthenticator(std::string auth_id, std::string secret, Dict auth_extra);
};

};  // namespace xconn
