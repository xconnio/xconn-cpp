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
class List;
class Dict;

struct Value {
    using VariantType = std::variant<std::monostate,         // null
                                     int64_t,                // integer
                                     uint64_t,               // unsigned integer
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

    Value(double v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(const std::string& v) : data(v) {}
    Value(const char* v) : data(std::string(v)) {}
    Value(const Bytes& v) : data(v) {}
    Value(const std::shared_ptr<List>& v) : data(v) {}
    Value(const std::shared_ptr<Dict>& v) : data(v) {}

    template <typename T, typename Decayed = std::decay_t<T>,
              typename = std::enable_if_t<!std::is_same_v<Decayed, Value> && std::is_constructible_v<VariantType, T>>>
    Value(T&& val) : data(std::forward<T>(val)) {}

    std::optional<int64_t> getInt64() const {
        if (std::holds_alternative<int64_t>(data)) return std::get<int64_t>(data);
        return std::nullopt;
    }

    std::optional<uint64_t> getUInt64() const {
        if (std::holds_alternative<uint64_t>(data)) return std::get<uint64_t>(data);
        return std::nullopt;
    }

    std::optional<double> getDouble() const {
        if (std::holds_alternative<double>(data)) return std::get<double>(data);
        return std::nullopt;
    }

    std::optional<bool> getBool() const {
        if (std::holds_alternative<bool>(data)) return std::get<bool>(data);
        return std::nullopt;
    }

    std::optional<std::string> getString() const {
        if (std::holds_alternative<std::string>(data)) return std::get<std::string>(data);
        return std::nullopt;
    }

    std::optional<Bytes> getBytes() const {
        if (std::holds_alternative<Bytes>(data)) return std::get<Bytes>(data);
        return std::nullopt;
    }

    std::shared_ptr<List> getList() const {
        if (std::holds_alternative<std::shared_ptr<List>>(data)) return std::get<std::shared_ptr<List>>(data);
        return nullptr;
    }

    std::shared_ptr<Dict> getDict() const {
        if (std::holds_alternative<std::shared_ptr<Dict>>(data)) return std::get<std::shared_ptr<Dict>>(data);
        return nullptr;
    }

    bool isNull() const { return std::holds_alternative<std::monostate>(data); }
};

class Dict : public std::unordered_map<std::string, Value> {
   public:
    using std::unordered_map<std::string, Value>::unordered_map;

    std::optional<Value> get(const std::string& key) const {
        auto it = find(key);
        if (it == end()) return std::nullopt;
        return it->second;
    }

    std::optional<std::string> getString(const std::string& key) const { return get(key)->getString(); }
    std::optional<bool> getBool(const std::string& key) const { return get(key)->getBool(); }
    std::optional<int64_t> getInt64(const std::string& key) const { return get(key)->getInt64(); }
    std::optional<uint64_t> getUInt64(const std::string& key) const { return get(key)->getUInt64(); }
    std::optional<double> getDouble(const std::string& key) const { return get(key)->getDouble(); }
    std::optional<Bytes> getBytes(const std::string& key) const { return get(key)->getBytes(); }

    std::optional<std::shared_ptr<List>> getList(const std::string& key) const {
        auto val = get(key);
        if (!val) return std::nullopt;

        auto list = val->getList();
        if (!list) return std::nullopt;

        return list;
    }

    std::optional<std::shared_ptr<Dict>> getDict(const std::string& key) const {
        auto val = get(key);
        if (!val) return std::nullopt;

        auto dict = val->getDict();
        if (!dict) return std::nullopt;

        return dict;
    }
};

class List : public std::vector<Value> {
   public:
    using std::vector<Value>::vector;

    std::optional<Value> get(int index) const {
        if (index < 0 || index >= size()) return std::nullopt;
        return at(index);
    }

    std::optional<std::string> getString(int index) const { return get(index)->getString(); }
    std::optional<bool> getBool(int index) const { return get(index)->getBool(); }
    std::optional<int64_t> getInt64(int index) const { return get(index)->getInt64(); }
    std::optional<int64_t> getUInt64(int index) const { return get(index)->getUInt64(); }
    std::optional<double> getDouble(int index) const { return get(index)->getDouble(); }
    std::optional<Bytes> getBytes(int index) const { return get(index)->getBytes(); }

    std::optional<std::shared_ptr<List>> getList(int index) const {
        auto val = get(index);
        if (!val) return std::nullopt;

        auto list = val->getList();
        if (!list) return std::nullopt;

        return list;
    }

    std::optional<std::shared_ptr<Dict>> getDict(int index) const {
        auto val = get(index);
        if (!val) return std::nullopt;

        auto dict = val->getDict();
        if (!dict) return std::nullopt;

        return dict;
    }
};

inline std::shared_ptr<List> make_list(std::initializer_list<Value> items) { return std::make_shared<List>(items); }

inline std::shared_ptr<Dict> make_dict(std::initializer_list<std::pair<std::string, Value>> init) {
    auto dict = std::make_shared<Dict>();
    for (const auto& [key, val] : init) {
        dict->emplace(key, val);
    }
    return dict;
}

std::ostream& operator<<(std::ostream& os, const List& list);
std::ostream& operator<<(std::ostream& os, const Dict& dict);
std::ostream& operator<<(std::ostream& os, const Value& v);

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

class ArgsHelper {
   public:
    List args;
    explicit ArgsHelper(List args) : args(args) {}
    std::optional<std::string> argString(int index) const { return args.getString(index); }
    std::optional<bool> argBool(int index) const { return args.getBool(index); }
    std::optional<int64_t> argInt64(int index) const { return args.getInt64(index); }
    std::optional<uint64_t> argUInt64(int index) const { return args.getUInt64(index); }
    std::optional<double> argDouble(int index) const { return args.getDouble(index); }
    std::optional<Bytes> argBytes(int index) const { return args.getBytes(index); }

    std::optional<std::shared_ptr<List>> argList(int index) const {
        auto list = args.getList(index);
        if (!list) return std::nullopt;

        return list;
    }

    std::optional<std::shared_ptr<Dict>> argDict(int index) const {
        auto dict = args.getDict(index);
        if (!dict) return std::nullopt;

        return dict;
    }
};

class KwargsHelper {
   public:
    Dict kwargs;
    explicit KwargsHelper(Dict kwargs) : kwargs(kwargs) {}
    std::optional<std::string> kwargString(const std::string& key) const { return kwargs.getString(key); }
    std::optional<bool> kwargBool(const std::string& key) const { return kwargs.getBool(key); }
    std::optional<int64_t> kwargInt64(const std::string& key) const { return kwargs.getInt64(key); }
    std::optional<uint64_t> kwargUInt64(const std::string& key) const { return kwargs.getUInt64(key); }
    std::optional<double> kwargDouble(const std::string& key) const { return kwargs.getDouble(key); }
    std::optional<Bytes> kwargBytes(const std::string& key) const { return kwargs.getBytes(key); }

    std::optional<std::shared_ptr<List>> kwargList(const std::string& key) const {
        auto list = kwargs.getList(key);
        if (!list) return std::nullopt;

        return list;
    }

    std::optional<std::shared_ptr<Dict>> kwargDict(const std::string& key) const {
        auto dict = kwargs.getDict(key);
        if (!dict) return std::nullopt;

        return dict;
    }
};

class Invocation : public ArgsHelper, public KwargsHelper {
   public:
    Dict details;

    Invocation(void* c_invocation);
    Invocation(List args_, Dict kwargs_, Dict details_);
};

class Result : public ArgsHelper, public KwargsHelper {
   public:
    Dict details;

    Result();
    Result(void* c_result);
    Result(const Invocation& invocation);
    Result(List args_, Dict kwargs_, Dict details_);
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

class Event : public ArgsHelper, public KwargsHelper {
   public:
    List args;
    Dict kwargs;
    Dict details;

    Event(void* c_event);
    Event(List args_, Dict kwargs_, Dict details_);
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
