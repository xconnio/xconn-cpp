#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xconn {

enum class SerializerType_ { JSON = 1, MSGPACK = 2, CBOR = 3 };

struct Value_;  // forward declaration

using Bytes_ = std::vector<uint8_t>;
using List_ = std::vector<Value_>;
using Dict_ = std::unordered_map<std::string, Value_>;

struct Value_ {
    using VariantType = std::variant<std::monostate,          // null
                                     int,                     // integer
                                     double,                  // floating point
                                     std::string,             // string
                                     bool,                    // boolean
                                     Bytes_,                  // bytes
                                     std::shared_ptr<List_>,  // array
                                     std::shared_ptr<Dict_>   // object
                                     >;

    VariantType data;

    Value_() = default;

    template <typename T>
    Value_(T&& val) : data(std::forward<T>(val)) {}

    Value_(int v) : data(v) {}
    Value_(double v) : data(v) {}
    Value_(bool v) : data(v) {}
    Value_(const std::string& v) : data(v) {}
    Value_(const char* v) : data(std::string(v)) {}
    Value_(const Bytes_& v) : data(v) {}
    Value_(const std::shared_ptr<List_>& v) : data(v) {}
    Value_(const std::shared_ptr<Dict_>& v) : data(v) {}
};

};  // namespace xconn
