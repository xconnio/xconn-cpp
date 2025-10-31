#include "xconn_cpp/authenticators.hpp"

#include <iostream>
#include <string>
#include <wampproto.h>

#include "xconn_cpp/types.hpp"

namespace xconn {

Authenticator::Authenticator(std::string auth_id, std::string auth_data, Dict_ auth_extra)
    : auth_id(std::move(auth_id)), auth_data(std::move(auth_data)), auth_extra(std::move(auth_extra)) {}

AnonymousAuthenticator::AnonymousAuthenticator(std::string auth_id, Dict_ auth_extra)
    : Authenticator(std::move(auth_id), "", std::move(auth_extra)) {
    auth_method = "anonymous";
    authenticator = anonymous_authenticator_new(this->auth_id.c_str(), nullptr);
}

TicketAuthenticator::TicketAuthenticator(std::string auth_id, std::string ticket, Dict_ auth_extra)
    : Authenticator(std::move(auth_id), std::move(ticket), std::move(auth_extra)) {
    auth_method = "ticket";
    authenticator = ticket_authenticator_new(this->auth_id.c_str(), this->auth_data.c_str(), nullptr);
}

CryptosignAuthenticator::CryptosignAuthenticator(std::string auth_id, std::string private_key_hex, Dict_ auth_extra)
    : Authenticator(std::move(auth_id), std::move(private_key_hex), std::move(auth_extra)) {
    auth_method = "cryptosign";
    authenticator = cryptosign_authenticator_new(this->auth_id.c_str(), this->auth_data.c_str(), nullptr);
}

WAMPCRAAuthenticator::WAMPCRAAuthenticator(std::string auth_id, std::string secret, Dict_ auth_extra)
    : Authenticator(std::move(auth_id), std::move(secret), std::move(auth_extra)) {
    auth_method = "wampcra";
    authenticator = wampcra_authenticator_new(this->auth_id.c_str(), this->auth_data.c_str(), nullptr);
}

}  // namespace xconn
