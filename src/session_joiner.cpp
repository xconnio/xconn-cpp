#include "xconn_cpp/session_joiner.hpp"

#include <iostream>

#include "xconn_cpp/authenticators.hpp"
#include "xconn_cpp/base_session.hpp"
#include "xconn_cpp/socket_transport.hpp"
#include "xconn_cpp/types.hpp"
#include "xconn_cpp/url_parser.hpp"

#include "wampproto/joiner.h"
#include "wampproto/transports/rawsocket.h"
#include "wampproto/value.h"

SessionJoiner::SessionJoiner(xconn::Authenticator authenticator, xconn::SerializerType serializer_type)
    : authenticator_(authenticator), serializer_type_(serializer_type), serializer_(nullptr) {
    switch (serializer_type_) {
        case xconn::SerializerType::JSON:
            serializer_ = json_serializer_new();
            break;
        case xconn::SerializerType::MSGPACK:
            serializer_ = msgpack_serializer_new();
            break;
        case xconn::SerializerType::CBOR:
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

    Joiner* joiner = joiner_new(realm.c_str(), serializer_, authenticator_.authenticator);
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
