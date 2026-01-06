#include "xconn_cpp/internal/base_session.hpp"

namespace xconn {

BaseSession::BaseSession(std::shared_ptr<SocketTransport> transport, const SessionDetails* session_details,
                         Serializer* serializer)
    : transport_(transport), session_details_(session_details), serializer(serializer) {}

std::shared_ptr<SocketTransport> BaseSession::transport() const { return transport_; }

uint64_t BaseSession::id() const { return session_details_->session_id; }

const char* BaseSession::realm() const { return session_details_->realm; }

const char* BaseSession::authid() const { return session_details_->auth_id; }

const char* BaseSession::authrole() const { return session_details_->auth_role; }

// Send raw bytes to transport
void BaseSession::send(::Bytes& bytes) { transport_->write(bytes); }

// Receive raw bytes from transport
::Bytes BaseSession::receive() {
    std::vector<uint8_t> data = transport_->read();

    ::Bytes bytes;
    bytes.len = data.size();
    bytes.data = static_cast<uint8_t*>(malloc(bytes.len));
    if (bytes.data && !data.empty()) {
        std::memcpy(bytes.data, data.data(), bytes.len);
    }
    return bytes;
}

// Send a serialized message
void BaseSession::send_message(const Message* msg) {
    ::Bytes bytes = serializer->serialize(serializer, msg);
    send(bytes);
    free(bytes.data);  // assuming serializer allocated this
}

// Receive and deserialize a message
Message* BaseSession::receive_message() {
    ::Bytes bytes = receive();
    Message* msg = serializer->deserialize(serializer, bytes);
    free(bytes.data);  // free receive buffer
    return msg;
}

// Close the transport
void BaseSession::close() { transport_->close(); }
void BaseSession::shutdown() { transport_->shutdown(); }

}  // namespace xconn
