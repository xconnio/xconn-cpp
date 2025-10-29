#include <cassert>
#include <iostream>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/session.h"
#include "xconn_cpp/session_joiner.hpp"
#include "xconn_cpp/socket_transport.hpp"

void test_joiner() {
    std::string url = "unix:///tmp/nxt.sock";
    std::string realm = "realm1";
    xconn::TicketAuthenticator authenticator =
        xconn::TicketAuthenticator("ticket-user", "ticket-pass", xconn::HashMap());
    SessionJoiner joiner = SessionJoiner(authenticator, xconn::SerializerType::JSON);
    BaseSession session = joiner.join(url, realm);

    assert(session.session_details()->session_id);
}

int main() {
    Session session;

    std::cout << "=== Test 1: Simple CallRequest ===\n";
    session.Call("http://example.com/api")
        .Arg(42)
        .Arg("hello")
        .Arg(1.1)
        .Arg(true)
        .Kwarg("name", "Alice")
        .Kwarg("age", 30)
        .Do();

    std::cout << "=== Test 1 Complete ===\n";

    test_joiner();

    return 0;
}
