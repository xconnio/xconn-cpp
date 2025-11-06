#pragma once
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xconn {

enum class SerializerType { JSON = 1, MSGPACK = 2, CBOR = 3 };

struct Value;   // forward declaration
class Session;  // forward declaration

using Bytes = std::vector<uint8_t>;
using List = std::vector<Value>;
using Dict = std::unordered_map<std::string, Value>;

struct Value {
    using VariantType = std::variant<std::monostate,         // null
                                     int,                    // integer
                                     double,                 // floating point
                                     std::string,            // string
                                     bool,                   // boolean
                                     Bytes,                  // bytes
                                     std::shared_ptr<List>,  // array
                                     std::shared_ptr<Dict>   // object
                                     >;

    VariantType data;

    Value() = default;

    Value(const Value&) = default;
    Value(Value&&) noexcept = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) noexcept = default;

    template <typename T, typename Decayed = std::decay_t<T>,
              typename = std::enable_if_t<!std::is_same_v<Decayed, Value> && std::is_constructible_v<VariantType, T>>>
    Value(T&& val) : data(std::forward<T>(val)) {}

    Value(int v) : data(v) {}
    Value(double v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(const std::string& v) : data(v) {}
    Value(const char* v) : data(std::string(v)) {}
    Value(const Bytes& v) : data(v) {}
    Value(const std::shared_ptr<List>& v) : data(v) {}
    Value(const std::shared_ptr<Dict>& v) : data(v) {}

    std::optional<int> get_int() const {
        if (std::holds_alternative<int>(data)) return std::get<int>(data);
        return std::nullopt;
    }

    std::optional<double> get_double() const {
        if (std::holds_alternative<double>(data)) return std::get<double>(data);
        return std::nullopt;
    }

    std::optional<bool> get_bool() const {
        if (std::holds_alternative<bool>(data)) return std::get<bool>(data);
        return std::nullopt;
    }

    std::optional<std::string> get_string() const {
        if (std::holds_alternative<std::string>(data)) return std::get<std::string>(data);
        return std::nullopt;
    }

    std::optional<Bytes> get_bytes() const {
        if (std::holds_alternative<Bytes>(data)) return std::get<Bytes>(data);
        return std::nullopt;
    }

    std::shared_ptr<List> get_list() const {
        if (std::holds_alternative<std::shared_ptr<List>>(data)) return std::get<std::shared_ptr<List>>(data);
        return nullptr;
    }

    std::shared_ptr<Dict> get_dict() const {
        if (std::holds_alternative<std::shared_ptr<Dict>>(data)) return std::get<std::shared_ptr<Dict>>(data);
        return nullptr;
    }

    bool is_null() const { return std::holds_alternative<std::monostate>(data); }
};

inline std::shared_ptr<List> make_list(std::initializer_list<Value> items) { return std::make_shared<List>(items); }

inline std::shared_ptr<Dict> make_dict(std::initializer_list<std::pair<std::string, Value>> init) {
    auto dict = std::make_shared<Dict>();
    for (const auto& [key, val] : init) {
        dict->emplace(key, val);
    }
    return dict;
}

std::ostream& operator<<(std::ostream& os, const Value& v);
std::ostream& operator<<(std::ostream& os, const List& list);
std::ostream& operator<<(std::ostream& os, const Dict& dict);

template <typename T>
std::optional<T> find_from_map(int64_t request_id, std::unordered_map<uint64_t, T>& map, std::mutex& map_mutex,
                               bool erase = true) {
    std::optional<T> maybe;

    {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto it = map.find(request_id);
        if (it != map.end()) {
            maybe = std::move(it->second);
            if (erase) map.erase(it);
        }
    }

    return maybe;
}

class ApplicationError : public std::exception {
   private:
    std::string message_;
    List list_;
    Dict dict_;
    mutable std::string cached_message_;

   public:
    ApplicationError(std::string message, List list = {}, Dict dict = {})
        : message_(std::move(message)), list_(std::move(list)), dict_(std::move(dict)) {}

    const char* what() const noexcept override {
        try {
            std::ostringstream oss;
            oss << message_;

            // Use your existing pretty-printers directly
            if (!list_.empty()) {
                oss << ": " << list_;
            }

            if (!dict_.empty()) {
                oss << ": " << dict_;
            }

            cached_message_ = oss.str();
            return cached_message_.c_str();
        } catch (...) {
            return "ApplicationError: (failed to format message)";
        }
    }

    const List& list() const noexcept { return list_; }
    const Dict& dict() const noexcept { return dict_; }
};

struct Invocation {
    List args;
    Dict kwargs;
    Dict details;

    Invocation(void* c_invocation);
};

struct Result {
    List args;
    Dict kwargs;
    Dict details;

    Result(void* c_result);
    Result(const Invocation& invocation);
};

struct Registration {
    uint64_t registration_id;
    Session& session;

    Registration(Session& session, uint64_t registration_id) : session(session), registration_id(registration_id) {}

    void unregister();
};

using ProcedureHandler = std::function<Result(const Invocation&)>;

struct RegisterRequest {
    std::promise<Registration> promise;
    ProcedureHandler handler;

    RegisterRequest(std::promise<Registration> promise, ProcedureHandler handler)
        : promise(std::move(promise)), handler(std::move(handler)) {}
};

struct UnregisterRequest {
    std::promise<void> promise;
    uint64_t registration_id;

    UnregisterRequest(uint64_t registration_id, std::promise<void> promise)
        : registration_id(registration_id), promise(std::move(promise)) {}
};

// PubSub

struct Event {
    List args;
    Dict kwargs;
    Dict details;

    Event(void* c_event);
};

using EventHandler = std::function<void(const Event&)>;

struct Subscription {
    uint64_t subscription_id;
    Session& session;

    Subscription(Session& session, uint64_t subscription_id) : session(session), subscription_id(subscription_id) {}

    void unsubscribe();
};

struct SubscribeRequest {
    std::promise<Subscription> promise;
    EventHandler handler;
};

struct UnsubscribeRequest {
    std::promise<void> promise;
    uint64_t subscription_id;

    UnsubscribeRequest(uint64_t subscription_id, std::promise<void> promise)
        : subscription_id(subscription_id), promise(std::move(promise)) {}
};

};  // namespace xconn
