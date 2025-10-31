#include <cassert>
#include <memory>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/client.hpp"
#include "xconn_cpp/types.hpp"

void test_client_session_lifecycle();

int main() {
    test_client_session_lifecycle();

    return 0;
}

std::string url = "unix:///tmp/nxt.sock";
std::string realm = "realm1";
std::string ticket_auth_id = "ticket-user";
std::string ticket = "ticket-pass";

void test_client_session_lifecycle() {
    auto client = std::make_unique<Client>(TicketAuthenticator(ticket_auth_id, ticket, Dict_()), SerializerType_::JSON);

    auto session = client->connect(url, realm);

    assert(session->session_id > 0);
    assert(session->auth_id == ticket_auth_id);
    assert(session->realm == realm);
    session->leave();

    assert(!session->is_connected());
}
