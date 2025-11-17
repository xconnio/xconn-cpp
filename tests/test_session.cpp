#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/client.hpp"
#include "xconn_cpp/types.hpp"

using namespace xconn;

void test_client_session_lifecycle();
void test_call_request();
void test_subscripiton_request();
void test_all_authenticator_and_serializers();

int main() {
    test_client_session_lifecycle();
    test_call_request();
    test_subscripiton_request();
    test_all_authenticator_and_serializers();

    return 0;
}

std::string url = "unix:///tmp/nxt.sock";
std::string realm = "realm1";
std::string procedure = "io.xconn.backend.add2";

std::string ticket_auth_id = "ticket-user";
std::string ticket = "ticket-pass";

std::string cryptosign_auth_id = "cryptosign-user";
std::string private_key_hex = "150085398329d255ad69e82bf47ced397bcec5b8fbeecd28a80edbbd85b49081";

std::string wampcra_auth_id = "wamp-cra-user";
std::string secret = "cra-secret";

std::string wampcra_salty_auth_id = "wamp-cra-salt-user";
std::string salty_secret = "cra-salt-secret";

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
        num1 = args[0].get_int64().value();
        num2 = args[1].get_int64().value();
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
    if (result.args.size() > 0) sum = result.args[0].get_int64().value();

    assert(sum == total);

    registration.unregister();

    session->leave();

    assert(!session->is_connected());
}

int expected_age = 25;
void test_subscripiton_request() {
    auto client = std::make_unique<Client>(CryptosignAuthenticator(cryptosign_auth_id, private_key_hex, Dict()),

                                           SerializerType::JSON);

    auto session = client->connect(url, realm);

    auto invoked = false;
    auto subscription = session
                            ->Subscribe("xconn.io.subscribe",
                                        [&invoked](const Event& event) {
                                            auto age = event.kwarg_int("age");
                                            if (age) assert(age == expected_age);
                                            invoked = true;
                                        })
                            .Do();

    session->Publish("xconn.io.subscribe").Arg(1).Kwarg("age", expected_age).Acknowledge(true).Do();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(invoked);

    subscription.unsubscribe();

    session->leave();
}

static std::string to_string(xconn::SerializerType type) {
    switch (type) {
        case xconn::SerializerType::JSON:
            return "JSON";
        case xconn::SerializerType::CBOR:
            return "CBOR";
        case xconn::SerializerType::MSGPACK:
            return "MSGPACK";
        default:
            return "Unknown";
    }
}

void test_all_authenticator_and_serializers() {
    std::vector<std::unique_ptr<Authenticator>> authenticators;

    authenticators.push_back(std::make_unique<CryptosignAuthenticator>(cryptosign_auth_id, private_key_hex));
    authenticators.push_back(std::make_unique<TicketAuthenticator>(ticket_auth_id, ticket));
    authenticators.push_back(std::make_unique<WAMPCRAAuthenticator>(wampcra_auth_id, secret));
    authenticators.push_back(std::make_unique<WAMPCRAAuthenticator>(wampcra_salty_auth_id, salty_secret));

    std::vector<xconn::SerializerType> serializer_types = {
        xconn::SerializerType::JSON,
        xconn::SerializerType::CBOR,
        xconn::SerializerType::MSGPACK,
    };

    for (auto& authenticator : authenticators) {
        for (auto serializer_type : serializer_types) {
            auto client = Client(*authenticator, serializer_type);
            std::cout << "Auth Method: " << authenticator->auth_method << " Serializer: " << to_string(serializer_type)
                      << std::endl;
            auto session = client.connect(url, realm);

            Result result = session->Call(procedure).Arg(2).Arg(4).Do();

            int sum = result.arg_int(0).value();

            assert(sum == 6);

            session->leave();
        }
    }
}
