#include "xconn_cpp/session.hpp"

#include <iostream>
#include <memory>

#include "xconn_cpp/internal/base_session.hpp"

Session::Session(std::unique_ptr<BaseSession> base_session) : base_session_(std::move(base_session)) {
    wamp_session = session_new(base_session->serializer);
    id_generator = id_generator_new();
}

void Session::send_message(Message* msg) {
    Bytes bytes = wamp_session->send_message(wamp_session, msg);
    base_session_->send(bytes);
}

void Session::process_incoming_message(Message* msg) {}

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
