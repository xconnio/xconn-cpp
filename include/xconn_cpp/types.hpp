#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xconn {

enum class SerializerType { JSON = 1, MSGPACK = 2, CBOR = 3 };

struct Value;  // forward declaration

using Bytes = std::vector<uint8_t>;
using List = std::vector<Value>;
using HashMap = std::unordered_map<std::string, Value>;

struct Value {
    using VariantType = std::variant<std::monostate,           // null
                                     int,                      // integer
                                     double,                   // floating point
                                     std::string,              // string
                                     bool,                     // boolean
                                     Bytes,                    // bytes
                                     std::shared_ptr<List>,    // array
                                     std::shared_ptr<HashMap>  // object
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
    Value(const std::shared_ptr<HashMap>& v) : data(v) {}
};

};  // namespace xconn
