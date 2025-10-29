#pragma once

#include <iostream>
#include <string>

#include "xconn_cpp/types.hpp"

#include "wampproto/authenticators/anonymous.h"
#include "wampproto/authenticators/authenticator.h"
#include "wampproto/authenticators/cryptosign.h"
#include "wampproto/authenticators/ticket.h"
#include "wampproto/authenticators/wampcra.h"

namespace xconn {

class Authenticator {
   public:
    virtual ~Authenticator() = default;

    Authenticator(std::string auth_id, std::string auth_data, xconn::HashMap auth_extra)
        : auth_id(std::move(auth_id)), auth_data(std::move(auth_data)), auth_extra(std::move(auth_extra)) {}

    std::string auth_id;
    xconn::HashMap auth_extra;
    std::string auth_method;
    std::string auth_data;

    ClientAuthenticator* authenticator;
};

class AnonymousAuthenticator : public Authenticator {
   public:
    AnonymousAuthenticator(std::string auth_id, xconn::HashMap auth_extra)
        : Authenticator(std::move(auth_id), "", std::move(auth_extra)) {
        auth_method = "anonymous";
        authenticator = anonymous_authenticator_new(auth_id.c_str(), NULL);
    }
};

class TicketAuthenticator : public Authenticator {
   public:
    TicketAuthenticator(std::string auth_id, std::string ticket, xconn::HashMap auth_extra)
        : Authenticator(std::move(auth_id), std::move(ticket), std::move(auth_extra)) {
        auth_method = "ticket";
        authenticator = ticket_authenticator_new(this->auth_id.c_str(), this->auth_data.c_str(), NULL);
    }
};

class CryptosignAuthenticator : public Authenticator {
   public:
    CryptosignAuthenticator(std::string auth_id, std::string private_key_hex, xconn::HashMap auth_extra)
        : Authenticator(std::move(auth_id), std::move(private_key_hex), std::move(auth_extra)) {
        auth_method = "cryptosign";
        authenticator = cryptosign_authenticator_new(this->auth_id.c_str(), this->auth_data.c_str(), NULL);
    }
};

class WAMPCRAAuthenticator : public Authenticator {
   public:
    WAMPCRAAuthenticator(std::string auth_id, std::string secret, xconn::HashMap auth_extra)
        : Authenticator(std::move(auth_id), std::move(secret), std::move(auth_extra)) {
        auth_method = "wampcra";
        authenticator = wampcra_authenticator_new(this->auth_id.c_str(), this->auth_data.c_str(), NULL);
    }
};

};  // namespace xconn
