#include "xconn_cpp/session.hpp"

#include <cstdint>
#include <exception>
#include <format>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include <sys/stat.h>

#include "xconn_cpp/internal/base_session.hpp"
#include "xconn_cpp/internal/types.hpp"
#include "xconn_cpp/types.hpp"

#include "wampproto/dict.h"
#include "wampproto/messages/publish.h"
#include "wampproto/messages/published.h"
#include "wampproto/messages/subscribe.h"
#include "wampproto/messages/subscribed.h"
#include "wampproto/value.h"

namespace xconn {

Session::Session(std::unique_ptr<BaseSession> base_session)
    : base_session_(std::move(base_session)),
      session_id(base_session->id()),
      auth_id(base_session->authid()),
      realm(base_session->realm()),
      auth_role(base_session->authrole()) {
    wamp_session = session_new(base_session_->serializer);
    id_generator = id_generator_new();

    pool_ = std::make_unique<ThreadPool>();

    recv_thread_ = std::thread(&Session::wait, this);
}

Session::~Session() {
    if (is_connected()) base_session_->close();
    running_ = false;
    if (recv_thread_.joinable()) recv_thread_.join();
}

int Session::leave() {
    std::string reason = "wamp.close.close_realm";
    Goodbye* goodbye = goodbye_new(create_dict(), reason.c_str());

    std::future<int> future = goodbye_promise.get_future();

    send_message((Message*)goodbye);

    return wait_with_timeout(future, TIMEOUT_SECONDS);
}

bool Session::is_connected() { return running_; }

void Session::send_message(Message* msg) {
    ::Bytes bytes = wamp_session->send_message(wamp_session, msg);
    if (is_connected()) {
        base_session_->send(bytes);
        return;
    }

    throw std::runtime_error("Connection closed");
}

void Session::process_incoming_message(Message* msg) {
    std::cout << msg->message_type << std::endl;
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

            auto maybe_promise = take_promise_from_map<Result>(request_id, call_requests_, call_requests_mutex_);
            if (maybe_promise.has_value()) maybe_promise->set_value(Result(result));
            break;
        }
        case MESSAGE_TYPE_REGISTERED: {
            ::Registered* registered = (::Registered*)msg;
            int64_t request_id = registered->request_id;

            auto request = find_from_map(request_id, register_requests_, register_requests_mutex_, true);

            if (request.has_value()) {
                {
                    std::lock_guard<std::mutex> lock(registrations_mutex_);
                    registrations_.emplace(registered->registration_id, std::move(request->handler));
                }

                Registration registeration(*this, registered->registration_id);
                request->promise.set_value(registeration);
            }
            break;
        }
        case MESSAGE_TYPE_UNREGISTERED: {
            ::Unregister* unregister = (::Unregister*)msg;
            int64_t request_id = unregister->request_id;

            auto request = find_from_map(request_id, unregister_requests_, unregister_requests_mutex_, true);
            if (request.has_value()) {
                find_from_map(request->registration_id, registrations_, registrations_mutex_, true);
                request->promise.set_value();
            }
            break;
        }
        case MESSAGE_TYPE_INVOCATION: {
            ::Invocation* invok = (::Invocation*)msg;
            auto handler = find_from_map(invok->registration_id, registrations_, registrations_mutex_, false);
            if (handler.has_value()) {
                Invocation invocation = Invocation(invok);
                pool_->enqueue([this, handler, invocation = std::move(invocation), invok]() mutable {
                    try {
                        Result result = (*handler)(invocation);

                        ::List* yield_args = vector_to_list(result.args);
                        ::Dict* yield_kwargs = unordered_map_to_dict(result.kwargs);
                        ::Dict* yield_options = unordered_map_to_dict(result.details);

                        Yield* yield = yield_new(invok->request_id, yield_options, yield_args, yield_kwargs);

                        base_session_->send_message((::Message*)yield);
                    } catch (const std::exception& e) {
                        // optional: handle errors gracefully
                        std::cerr << "Registration Handler execution failed: " << e.what() << std::endl;
                    }
                });
            }

            break;
        }
        case MESSAGE_TYPE_PUBLISHED: {
            ::Published* published = (::Published*)msg;
            int64_t request_id = published->request_id;

            auto maybe_promise = take_promise_from_map<void>(request_id, publish_requests_, publish_requests_mutex_);
            if (maybe_promise.has_value()) maybe_promise->set_value();

            break;
        }
        case MESSAGE_TYPE_SUBSCRIBED: {
            ::Subscribed* subscribed = (::Subscribed*)msg;
            int64_t request_id = subscribed->request_id;
            auto request = find_from_map(request_id, subscribe_requests_, subscribe_requests_mutex_, true);
            if (request.has_value()) {
                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_.emplace(subscribed->subscription_id, std::move(request->handler));
            }

            auto subscription = Subscription(*this, subscribed->subscription_id);

            request->promise.set_value(subscription);
            break;
        }
        case MESSAGE_TYPE_EVENT: {
            ::Event* c_event = (::Event*)msg;
            int64_t subscription_id = c_event->subscription_id;

            auto handler = find_from_map(subscription_id, subscriptions_, subscriptions_mutex_, false);
            if (handler) {
                Event event = Event(c_event);

                pool_->enqueue([this, handler, event]() mutable {
                    try {
                        handler->operator()(event);
                    } catch (const std::exception& e) {
                        std::cerr << "Subscription Handler execution failed: " << e.what() << std::endl;
                    }
                });
            }
            std::cout << "FREEING C_EVENT" << std::endl;
            c_event->base.free((Message*)c_event);
            std::cout << "FREEING C_EVENT" << std::endl;

            break;
        }
        case MESSAGE_TYPE_ERROR: {
            ::Error* error = (::Error*)msg;
            std::string invalid_error_message =
                std::format("Received {} message for invalid request ID", error->message_type);
            std::exception_ptr invalid_error = std::make_exception_ptr(std::runtime_error(invalid_error_message));

            std::exception_ptr application_error = std::make_exception_ptr(std::runtime_error(error->uri));

            int64_t request_id = error->request_id;

            switch (error->message_type) {
                case MESSAGE_TYPE_CALL: {
                    auto promise = take_promise_from_map<Result>(request_id, call_requests_, call_requests_mutex_);

                    std::exception_ptr exception = promise.has_value() ? application_error : invalid_error;
                    promise->set_exception(exception);

                    break;
                }
                case MESSAGE_TYPE_REGISTER: {
                    auto request = find_from_map(request_id, register_requests_, register_requests_mutex_, true);
                    std::exception_ptr exception = request.has_value() ? application_error : invalid_error;
                    request->promise.set_exception(exception);
                    break;
                }
                default:
                    std::cout << "Result CODE: " << error->message_type << std::endl;
            }

            throw std::runtime_error(error->uri);
        }

        default: {
            std::cout << "Message ID: " << msg->message_type << std::endl;
        }
    }
}

void Session::wait() {
    while (running_) {
        try {
            Message* msg = base_session_->receive_message();
            if (!msg) continue;

            process_incoming_message(msg);
        } catch (const std::system_error& e) {
            std::cerr << "System closed the connection" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in wait(): " << e.what() << '\n';
        } catch (...) {
            std::cerr << "Unknown exception in wait()" << std::endl;
        }
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

    return session_.wait_with_timeout(future, TIMEOUT_SECONDS);
}

Session::CallRequest Session::Call(const std::string& uri) { return CallRequest(*this, uri); }

Session::RegisterRequest::RegisterRequest(Session& session, const std::string uri, ProcedureHandler handler)
    : uri(std::move(uri)), session_(session), handler_(std::move(handler)) {}

Session::RegisterRequest& Session::RegisterRequest::Option(const std::string& key, const Value& value) {
    options[key] = value;
    return *this;
}

Registration Session::RegisterRequest::Do() const {
    ::Dict* regsiter_options = unordered_map_to_dict(options);
    int64_t request_id = session_.id_generator->next();

    ::Register* r = register_new(request_id, regsiter_options, uri.c_str());

    std::promise<Registration> promise;
    std::future<Registration> future = promise.get_future();

    xconn::RegisterRequest request(std::move(promise), std::move(handler_));
    {
        std::lock_guard<std::mutex> lock(session_.register_requests_mutex_);
        session_.register_requests_.emplace(request_id, std::move(request));
    }

    session_.send_message((Message*)r);

    return session_.wait_with_timeout(future, TIMEOUT_SECONDS);
}

void Session::Unregister(uint64_t registration_id) {
    int64_t request_id = id_generator->next();

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    ::Unregister* unregister = unregister_new(request_id, registration_id);

    UnregisterRequest request(registration_id, std::move(promise));

    {
        std::lock_guard<std::mutex> lock(unregister_requests_mutex_);
        unregister_requests_.emplace(request_id, std::move(request));
    }

    send_message((Message*)unregister);

    return wait_with_timeout(future, TIMEOUT_SECONDS);
}

Session::RegisterRequest Session::Register(const std::string& uri, ProcedureHandler handler) {
    return RegisterRequest(*this, std::move(uri), std::move(handler));
}

void Registration::unregister() { return session.Unregister(registration_id); }

Session::PublishRequest::PublishRequest(Session& session, const std::string& uri)
    : session_(session), uri_(std::move(uri)) {}

Session::PublishRequest& Session::PublishRequest::Arg(const Value& arg) {
    args_.push_back(arg);
    return *this;
}

Session::PublishRequest& Session::PublishRequest::Kwarg(const std::string& key, const Value& value) {
    kwargs_[key] = value;
    return *this;
}

Session::PublishRequest& Session::PublishRequest::Option(const std::string& key, const Value& value) {
    options_[key] = value;
    return *this;
}

Session::PublishRequest& Session::PublishRequest::Acknowledge(bool value) {
    options_["acknowledge"] = value;
    return *this;
}

void Session::PublishRequest::Do() const {
    ::List* publish_args = vector_to_list(args_);
    ::Dict* publish_kwargs = unordered_map_to_dict(kwargs_);
    ::Dict* publish_options = unordered_map_to_dict(options_);
    int64_t request_id = session_.id_generator->next();

    ::Publish* publish = publish_new(request_id, publish_options, uri_.c_str(), publish_args, publish_kwargs);

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    auto acknowledge = false;

    auto it = options_.find("acknowledge");
    if (it != options_.end()) acknowledge = it->second.get_bool().value();

    if (acknowledge) {
        std::lock_guard<std::mutex> lock(session_.publish_requests_mutex_);
        session_.publish_requests_.emplace(request_id, std::move(promise));
    }

    session_.send_message((Message*)publish);

    if (acknowledge) session_.wait_with_timeout(future, TIMEOUT_SECONDS);
}

Session::PublishRequest Session::Publish(const std::string& uri) { return PublishRequest(*this, std::move(uri)); }

Session::SubscribeRequest::SubscribeRequest(Session& session, std::string topic, EventHandler handler)
    : session_(session), topic_(std::move(topic)), handler_(std::move(handler)) {}

Session::SubscribeRequest& Session::SubscribeRequest::Option(std::string key, xconn::Value value) {
    options_[std::move(key)] = std::move(value);
    return *this;
}

Subscription Session::SubscribeRequest::Do() const {
    ::Dict* options = unordered_map_to_dict(options_);
    int64_t request_id = session_.id_generator->next();

    ::Subscribe* subscribe = subscribe_new(request_id, options, topic_.c_str());

    std::promise<Subscription> promise;
    std::future<Subscription> future = promise.get_future();

    auto request = xconn::SubscribeRequest(std::move(promise), handler_);
    {
        std::lock_guard<std::mutex> lock(session_.subscribe_requests_mutex_);
        session_.subscribe_requests_.emplace(request_id, std::move(request));
    }

    session_.send_message((Message*)subscribe);

    return session_.wait_with_timeout(future, TIMEOUT_SECONDS);
}

Session::SubscribeRequest Session::Subscribe(std::string topic, EventHandler handler) {
    return SubscribeRequest(*this, std::move(topic), std::move(handler));
}

}  // namespace xconn
