#include "xconn_cpp/session.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <sys/stat.h>

#include "xconn_cpp/internal/base_session.hpp"
#include "xconn_cpp/internal/types.hpp"
#include "xconn_cpp/types.hpp"

namespace xconn {

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
    ::Bytes bytes = wamp_session->send_message(wamp_session, msg);
    if (is_connected()) {
        base_session_->send(bytes);
        return;
    }

    throw std::runtime_error("Connection closed:");
}

void Session::process_incoming_message(Message* msg) {
    switch (msg->message_type) {
        case MESSAGE_TYPE_GOODBYE: {
            goodbye_promise.set_value(0);
            running_ = false;
            base_session_->close();
            break;
        }
        case MESSAGE_TYPE_RESULT: {
            ::Result* result = (::Result*)msg;
            int64_t request_id = result->request_id;

            std::optional<std::promise<Result>> maybe_promise;

            {
                std::lock_guard<std::mutex> lock(call_requests_mutex_);
                auto it = call_requests_.find(request_id);
                if (it != call_requests_.end()) {
                    maybe_promise = std::move(it->second);
                    call_requests_.erase(it);
                }
            }

            if (maybe_promise.has_value()) maybe_promise->set_value(Result(result));
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

Session::CallRequest::CallRequest(Session& session, const std::string uri) : uri(std::move(uri)), session_(session) {}

Session::CallRequest& Session::CallRequest::Arg(const Value& arg) {
    args.push_back(arg);
    return *this;
}

Session::CallRequest& Session::CallRequest::Kwarg(const std::string& key, const Value& value) {
    kwargs[key] = value;
    return *this;
}

Session::CallRequest& Session::CallRequest::Option(const std::string& key, const Value& value) {
    options[key] = value;
    return *this;
}

Result Session::CallRequest::Do() const {
    std::cout << "Calling URI: " << uri << "\n";

    ::List* call_args = vector_to_list(args);
    ::Dict* call_kwargs = unordered_map_to_dict(kwargs);
    ::Dict* call_options = unordered_map_to_dict(options);
    int64_t request_id = session_.id_generator->next();

    ::Call* call = call_new(request_id, call_options, uri.c_str(), call_args, call_kwargs);

    std::promise<Result> promise;
    std::future<Result> future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(session_.call_requests_mutex_);
        session_.call_requests_.emplace(request_id, std::move(promise));
    }

    session_.send_message((Message*)call);

    return session_.wait_with_timeout(future, 10);
}

Session::CallRequest Session::Call(const std::string& uri) { return CallRequest(*this, uri); }

}  // namespace xconn
