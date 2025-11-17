#include "xconn_cpp/internal/socket_transport.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>
#include <wampproto.h>

#include "xconn_cpp/transports.hpp"
#include "xconn_cpp/types.hpp"
#include "xconn_cpp/url_parser.hpp"

namespace xconn {

SocketTransport::SocketTransport(asio::io_context& io, UrlParser& url) : transport_(create_transport(io, url)) {}

SocketTransport::~SocketTransport() { close(); }

std::shared_ptr<SocketTransport> SocketTransport::Create(std::string& url) {
    static asio::io_context io;
    UrlParser parser = parse_url(url);
    return std::make_shared<SocketTransport>(io, parser);
}

bool SocketTransport::connect(const std::string& host, const std::string& port, xconn::SerializerType serializer_type,
                              int max_msg_size) {
    try {
        transport_->connect(host, port);

        uint8_t hs_request[4];
        ::SerializerType c_serializer_type = (::SerializerType)serializer_type;
        Handshake* hs = handshake_new(c_serializer_type, max_msg_size);
        if (send_handshake(hs, hs_request) != 0) {
            handshake_free(hs);
            return false;
        }

        transport_->write(hs_request, 4);

        uint8_t hs_response[4];
        if (!recv_exactly(hs_response, 4)) {
            handshake_free(hs);
            return false;
        }

        int err = 0;
        Handshake* response = receive_handshake(hs_response, &err);
        if (!response || err != 0 || response->serializer != c_serializer_type) {
            handshake_free(hs);
            handshake_free(response);
            return false;
        }

        handshake_free(hs);
        handshake_free(response);
        return true;
    } catch (std::exception& e) {
        std::cerr << "Connect error: " << e.what() << std::endl;
        return false;
    }
}

bool SocketTransport::recv_exactly(uint8_t* buffer, size_t n) {
    if (transport_->read(buffer, n) != n) {
        return false;
    }
    return true;
}

std::vector<uint8_t> SocketTransport::read() {
    uint8_t header_bytes[4];
    if (!recv_exactly(header_bytes, 4)) return {};

    MessageHeader* header = receive_message_header(header_bytes);
    if (!header) return {};

    std::vector<uint8_t> payload(header->length);
    if (!recv_exactly(payload.data(), payload.size())) {
        message_header_free(header);
        return {};
    }

    message_header_free(header);
    return payload;
}

::Bytes SocketTransport::read_bytes() {
    std::vector<uint8_t> data = read();
    std::string str(data.begin(), data.end());
    ::Bytes bytes;
    bytes.len = data.size();
    bytes.data = static_cast<uint8_t*>(malloc(bytes.len));
    memcpy(bytes.data, data.data(), bytes.len);

    return bytes;
}

bool SocketTransport::write(::Bytes& bytes) {
    std::lock_guard<std::mutex> lock(write_mutex_);

    std::cout << bytes.data << std::endl;

    MessageType MSG_TYPE_WAMP = MESSAGE_WAMP;
    MessageHeader* header = message_header_new(MSG_TYPE_WAMP, bytes.len);
    if (!header) return false;

    uint8_t header_bytes[4];
    send_message_header(header, header_bytes);

    try {
        transport_->write(header_bytes, 4);
        transport_->write(bytes.data, bytes.len);

        message_header_free(header);
        return true;
    } catch (std::exception& e) {
        std::cerr << "Write error: " << e.what() << std::endl;
        message_header_free(header);
        return false;
    }
}

void SocketTransport::close() { transport_->close(); }

bool SocketTransport::is_connected() const { return transport_->is_connected(); }
}  // namespace xconn
