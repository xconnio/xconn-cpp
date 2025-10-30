#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "xconn_cpp/types.hpp"

extern "C" {
typedef struct wampproto_Session wampproto_Session;
typedef struct IDGenerator IDGenerator;
typedef struct Message Message;
}

class BaseSession;

using namespace xconn;

class Session {
    Session(std::unique_ptr<BaseSession> base_session);

   public:
    class CallRequest {
       public:
        explicit CallRequest(std::string uri);

        CallRequest& Arg(const xconn::Value_& arg);
        CallRequest& Kwarg(const std::string& key, const Value_& value);
        CallRequest& Option(const std::string& key, const Value_& value);

        void Do() const;

       private:
        std::string uri;
        List_ args;
        Dict_ kwargs;
        Dict_ options;
    };

    CallRequest Call(const std::string& uri);

   private:
    std::unique_ptr<BaseSession> base_session_;
    wampproto_Session* wamp_session;
    IDGenerator* id_generator;
    std::atomic<bool> running_{true};

    void send_message(Message* msg);
    void process_incoming_message(Message* msg);
    void wait();
};
