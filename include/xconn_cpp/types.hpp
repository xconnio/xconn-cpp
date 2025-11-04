#pragma once
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <optional>
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

    template <typename T>
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

template <typename T>
std::optional<std::promise<T>> take_promise_from_map(int64_t request_id,
                                                     std::unordered_map<uint64_t, std::promise<T>>& promise_map,
                                                     std::mutex& map_mutex) {
    std::optional<std::promise<T>> maybe_promise;

    {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto it = promise_map.find(request_id);
        if (it != promise_map.end()) {
            maybe_promise = std::move(it->second);
            promise_map.erase(it);
        }
    }

    return maybe_promise;
}

template <typename T>
std::optional<T> find_from_map(int64_t request_id, std::unordered_map<uint64_t, T>& map, std::mutex& map_mutex,
                               bool erase) {
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

};  // namespace xconn
