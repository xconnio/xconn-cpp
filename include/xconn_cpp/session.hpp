#pragma once
#include <atomic>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

#include <sys/types.h>

#include "xconn_cpp/internal/thread_pool.hpp"
#include "xconn_cpp/types.hpp"

extern "C" {
typedef struct wampproto_Session wampproto_Session;
typedef struct IDGenerator IDGenerator;
typedef struct Message Message;
}

namespace xconn {

constexpr int TIMEOUT_SECONDS = 10;
constexpr const char* ERROR_RUNTIME_ERROR = "wamp.error.runtime_error";

enum class SessionState {
    CONNECTED,
    LEAVING,
    DISCONNECTED,
};

class BaseSession;
class ThreadPool;

class Session {
   public:
    Session(std::unique_ptr<BaseSession> base_session);
    ~Session();

    int64_t session_id;
    const std::string realm;
    const std::string auth_id;
    const std::string auth_role;

    bool is_connected();
    int leave();

    class CallRequest {
       public:
        CallRequest(Session& session, std::string uri);

        CallRequest& Arg(xconn::Value arg);
        CallRequest& Kwarg(std::string key, xconn::Value value);
        CallRequest& Option(std::string key, xconn::Value value);

        Result Do() const;

       private:
        Session& session_;
        std::string procedure_;
        List args_;
        Dict kwargs_;
        Dict options_;
    };

    CallRequest Call(std::string uri);

    class RegisterRequest {
       public:
        RegisterRequest(Session& session, std::string uri, ProcedureHandler handler);

        RegisterRequest& Option(std::string key, xconn::Value value);

        Registration Do() const;

       private:
        Session& session_;
        std::string procedure_;
        ProcedureHandler handler_;
        Dict options;
    };

    RegisterRequest Register(std::string procedure, ProcedureHandler handler);

    void Unregister(uint64_t registration_id);

    class PublishRequest {
       public:
        PublishRequest(Session& session, std::string uri);

        PublishRequest& Arg(xconn::Value arg);
        PublishRequest& Kwarg(std::string key, xconn::Value value);
        PublishRequest& Option(std::string key, xconn::Value value);
        PublishRequest& Acknowledge(bool value);

        void Do() const;

       private:
        Session& session_;
        std::string topic_;
        List args_;
        Dict kwargs_;
        Dict options_;
    };

    PublishRequest Publish(std::string topic);

    class SubscribeRequest {
       public:
        SubscribeRequest(Session& session, std::string topic, EventHandler handler);

        SubscribeRequest& Option(std::string key, xconn::Value value);

        Subscription Do() const;

       private:
        Session& session_;
        std::string topic_;
        EventHandler handler_;
        Dict options_;
    };

    SubscribeRequest Subscribe(std::string topic, EventHandler handler);

    void Unsubscribe(uint64_t subscription_id);

   private:
    std::unique_ptr<BaseSession> base_session_;
    wampproto_Session* wamp_session;
    IDGenerator* id_generator;

    std::thread recv_thread_;
    std::atomic<SessionState> state_{SessionState::CONNECTED};

    std::mutex send_mutex_;
    std::optional<std::promise<int>> goodbye_promise_;
    std::atomic<bool> goodbye_sent{false};

    std::unique_ptr<ThreadPool> pool_;

    std::mutex call_requests_mutex_;
    std::unordered_map<uint64_t, std::promise<Result>> call_requests_;

    std::mutex register_requests_mutex_;
    std::unordered_map<uint64_t, xconn::RegisterRequest> register_requests_;

    std::mutex registrations_mutex_;
    std::unordered_map<uint64_t, ProcedureHandler> registrations_;

    std::mutex unregister_requests_mutex_;
    std::unordered_map<uint64_t, UnregisterRequest> unregister_requests_;

    std::mutex publish_requests_mutex_;
    std::unordered_map<uint64_t, std::promise<void>> publish_requests_;

    std::mutex subscribe_requests_mutex_;
    std::unordered_map<uint64_t, xconn::SubscribeRequest> subscribe_requests_;

    std::mutex subscriptions_mutex_;
    std::unordered_map<uint64_t, EventHandler> subscriptions_;

    std::mutex unsubscribe_requests_mutex_;
    std::unordered_map<uint64_t, UnsubscribeRequest> unsubscribe_requests_;

    void send_message(Message* msg);
    void process_incoming_message(Message* msg);
    void wait();

    template <typename T>
    T wait_with_timeout(std::future<T>& future, int seconds) {
        auto status = future.wait_for(std::chrono::seconds(seconds));

        if (status == std::future_status::ready) {
            return future.get();
        } else if (status == std::future_status::timeout) {
            throw std::runtime_error("Timeout waiting for future result");
        } else {
            throw std::runtime_error("Unexpected future state");
        }
    }
};

}  // namespace xconn
