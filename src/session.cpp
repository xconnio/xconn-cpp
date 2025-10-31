#include "xconn_cpp/session.hpp"

#include <future>
#include <iostream>
#include <memory>

#include "xconn_cpp/internal/base_session.hpp"

Session::Session(std::unique_ptr<BaseSession> base_session)
    : base_session_(std::move(base_session)),
      session_id(base_session->id()),
      auth_id(base_session->authid()),
      realm(base_session->realm()),
      auth_role(base_session->authrole()) {
    wamp_session = session_new(base_session_->serializer);
    id_generator = id_generator_new();

    recv_thread_ = std::thread(&Session::wait, this);
}

Session::~Session() {
    running_ = false;
    if (recv_thread_.joinable()) recv_thread_.join();
}

int Session::leave() {
    std::string reason = "wamp.close.close_realm";
    Goodbye* goodbye = goodbye_new(create_dict(), reason.c_str());

    std::future<int> future = goodbye_promise.get_future();

    send_message((Message*)goodbye);

    return future.get();
}

bool Session::is_connected() { return running_; }

void Session::send_message(Message* msg) {
    Bytes bytes = wamp_session->send_message(wamp_session, msg);
    base_session_->send(bytes);
}

void Session::process_incoming_message(Message* msg) {
    switch (msg->message_type) {
        case MESSAGE_TYPE_GOODBYE: {
            goodbye_promise.set_value(0);
            running_ = false;
            base_session_->close();
            break;
        }
        default: {
            std::cout << "Message ID: " << msg->message_type << std::endl;
        }
    }
}

void Session::wait() {
    while (running_) {
        Message* msg = base_session_->receive_message();
        if (!msg) continue;

        process_incoming_message(msg);
    }
}

Session::CallRequest::CallRequest(std::string uri) : uri(std::move(uri)) {}

Session::CallRequest& Session::CallRequest::Arg(const Value_& arg) {
    args.push_back(arg);
    return *this;
}

Session::CallRequest& Session::CallRequest::Kwarg(const std::string& key, const Value_& value) {
    kwargs[key] = value;
    return *this;
}

Session::CallRequest& Session::CallRequest::Option(const std::string& key, const Value_& value) {
    options[key] = value;
    return *this;
}

void Session::CallRequest::Do() const {
    std::cout << "Calling URI: " << uri << "\n";

    std::cout << "Network call complete!\n";
}

Session::CallRequest Session::Call(const std::string& uri) { return CallRequest(uri); }
