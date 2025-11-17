#include <cassert>
#include <iostream>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/internal/base_session.hpp"
#include "xconn_cpp/internal/types.hpp"
#include "xconn_cpp/session_joiner.hpp"
#include "xconn_cpp/types.hpp"

#include "wampproto/dict.h"
#include "wampproto/value.h"

std::string url = "unix:///tmp/nxt.sock";
std::string realm = "realm1";
std::string ticket_auth_id = "ticket-user";
std::string ticket = "ticket-pass";

using namespace xconn;

void test_session_joiner_with_ticket() {
    TicketAuthenticator authenticator = TicketAuthenticator(ticket_auth_id, ticket, xconn::Dict());
    SessionJoiner joiner = SessionJoiner(authenticator, xconn::SerializerType::JSON);
    auto session = joiner.join(url, realm);

    assert(session->id() > 0);
}

std::string wampcra_auth_id = "wamp-cra-user";
std::string secret = "cra-secret";

void test_session_joiner_with_wampcra() {
    WAMPCRAAuthenticator authenticator = WAMPCRAAuthenticator(wampcra_auth_id, secret, xconn::Dict());

    SessionJoiner joiner = SessionJoiner(authenticator, xconn::SerializerType::CBOR);
    auto session = joiner.join(url, realm);

    assert(session->id() > 0);
}

int main() {
    test_session_joiner_with_ticket();
    test_session_joiner_with_wampcra();

    return 0;
}
