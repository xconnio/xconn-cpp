#include "xconn_cpp/session_joiner.hpp"

#include <iostream>

#include "xconn_cpp/base_session.hpp"
#include "xconn_cpp/socket_transport.hpp"
#include "xconn_cpp/url_parser.hpp"

#include "wampproto/authenticators/authenticator.h"
#include "wampproto/joiner.h"
#include "wampproto/transports/rawsocket.h"
#include "wampproto/value.h"

SessionJoiner::SessionJoiner(ClientAuthenticator* auth, SerializerType serializer)
    : authenticator_(auth), serializer_type_(serializer), serializer_(nullptr) {
    switch (serializer_type_) {
        case SERIALIZER_JSON:
            serializer_ = json_serializer_new();
            break;
        case SERIALIZER_MSGPACK:
            serializer_ = msgpack_serializer_new();
            break;
        case SERIALIZER_CBOR:
            serializer_ = cbor_serializer_new();
            break;
        default:
            throw std::invalid_argument("Unknown serializer type");
    }
}

SessionJoiner::~SessionJoiner() { free(serializer_); }

BaseSession SessionJoiner::join(std::string& uri, std::string& realm) {
    auto transport = SocketTransport::Create(uri);
    UrlParser parser = parse_url(uri);

    transport->connect(parser.host, parser.port, serializer_type_, MAX_MSG_SIZE);

    Joiner* joiner = joiner_new(const_cast<char*>(realm.c_str()), serializer_, authenticator_);
    Bytes hello = joiner->send_hello(joiner);
    transport->write(hello);

    while (true) {
        Bytes bytes = transport->read_bytes();

        Bytes to_send = joiner->receive(joiner, bytes);

        if (to_send.len == 0) {
            std::cout << "Successfully created WAMP Session" << std::endl;
            return BaseSession(transport, joiner->session_details, serializer_);
        }
        transport->write(to_send);
    }
}
