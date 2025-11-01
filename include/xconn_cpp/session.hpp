#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "xconn_cpp/internal/thread_pool.hpp"
#include "xconn_cpp/types.hpp"

extern "C" {
typedef struct wampproto_Session wampproto_Session;
typedef struct IDGenerator IDGenerator;
typedef struct Message Message;
}

namespace xconn {

constexpr int TIMEOUT_SECONDS = 10;

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

        CallRequest& Arg(const xconn::Value& arg);
        CallRequest& Kwarg(const std::string& key, const xconn::Value& value);
        CallRequest& Option(const std::string& key, const xconn::Value& value);

        Result Do() const;

       private:
        Session& session_;
        std::string uri;
        List args;
        Dict kwargs;
        Dict options;
    };

    CallRequest Call(const std::string& uri);

    class RegisterRequest {
       public:
        RegisterRequest(Session& session, std::string uri, ProcedureHandler handler);

        RegisterRequest& Option(const std::string& key, const xconn::Value& value);

        Registration Do() const;

       private:
        Session& session_;
        std::string uri;
        ProcedureHandler handler_;
        Dict options;
    };

    RegisterRequest Register(const std::string& uri, ProcedureHandler handler);

    void Unregister(uint64_t registration_id);

   private:
    std::unique_ptr<BaseSession> base_session_;
    wampproto_Session* wamp_session;
    IDGenerator* id_generator;

    std::thread recv_thread_;
    std::atomic<bool> running_{true};
    std::promise<int> goodbye_promise;

    std::unique_ptr<ThreadPool> pool_;

    std::mutex call_requests_mutex_;
    std::unordered_map<uint64_t, std::promise<Result>> call_requests_;

    std::mutex register_requests_mutex_;
    std::unordered_map<uint64_t, xconn::RegisterRequest> register_requests_;

    std::mutex registrations_mutex_;
    std::unordered_map<uint64_t, ProcedureHandler> registrations_;

    std::mutex unregister_requests_mutex_;
    std::unordered_map<uint64_t, UnregisterRequest> unregister_requests_;

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
