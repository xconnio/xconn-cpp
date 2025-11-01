#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/client.hpp"
#include "xconn_cpp/types.hpp"

using namespace xconn;

void test_client_session_lifecycle();
void test_call_request();

int main() {
    test_client_session_lifecycle();
    test_call_request();

    return 0;
}

std::string url = "unix:///tmp/nxt.sock";
std::string realm = "realm1";
std::string ticket_auth_id = "ticket-user";
std::string ticket = "ticket-pass";
std::string procedure = "io.xconn.backend.add2";

void test_client_session_lifecycle() {
    auto client = std::make_unique<Client>(TicketAuthenticator(ticket_auth_id, ticket, Dict()), SerializerType::JSON);

    auto session = client->connect(url, realm);

    Result result = session->Call(procedure).Arg(2).Arg(4).Do();

    assert(session->session_id > 0);
    assert(session->auth_id == ticket_auth_id);
    assert(session->realm == realm);
    session->leave();

    assert(!session->is_connected());
}

std::string call_procedure = "io.xconn.sum";
int num1 = 10;
int num2 = 5;
int total = num1 + num2;

ProcedureHandler procedure_handler = [](const Invocation& invocation) -> Result {
    int num1 = 0;
    int num2 = 0;
    auto args = invocation.args;
    if (args.size() == 2) {
        num1 = args[0].get_int().value();
        num2 = args[1].get_int().value();
    }

    Result result = Result(invocation);
    result.args = List{num1 + num2};

    return result;
};

void test_call_request() {
    auto client = std::make_unique<Client>(AnonymousAuthenticator("john", Dict()), SerializerType::JSON);

    auto session = client->connect(url, realm);

    auto registration = session->Register(call_procedure, procedure_handler).Do();

    auto result = session->Call(call_procedure).Arg(num1).Arg(num2).Do();

    int sum = 0;
    if (result.args.size() > 0) sum = result.args[0].get_int().value();

    assert(sum == total);

    registration.unregister();

    try {
        auto result = session->Call(call_procedure).Arg(num1).Arg(num2).Do();
        assert(false);
    } catch (...) {
        assert(true);
    }

    session->leave();

    assert(!session->is_connected());
}
