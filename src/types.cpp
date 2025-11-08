#include "xconn_cpp/types.hpp"

#include <type_traits>

#include "wampproto.h"

namespace xconn {

::Value* to_c_value(const Value& val);

::Value* to_c_list(const List& list) {
    ::Value* c_list = ::value_list(list.size());
    for (const auto& elem : list) {
        ::Value* c_val = to_c_value(elem);
        if (c_val) value_list_append(c_list, c_val);
    }
    return c_list;
}

::Value* to_c_dict(const HashMap& map) {
    ::Value* c_dict = ::value_dict();
    for (const auto& [key, val] : map) {
        ::Value* c_val = to_c_value(val);
        if (c_val) ::value_dict_append(c_dict, key.c_str(), c_val);
    }
    return c_dict;
}

::Value* to_c_value(const Value& v) {
    return std::visit(
        [](auto&& value) -> ::Value* {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, std::monostate>)
                return ::value_null();
            else if constexpr (std::is_same_v<T, int>)
                return ::value_int(value);
            else if constexpr (std::is_same_v<T, double>)
                return ::value_float(value);
            else if constexpr (std::is_same_v<T, std::string>)
                return ::value_str(value.c_str());
            else if constexpr (std::is_same_v<T, bool>)
                return ::value_bool(value ? 1 : 0);
            else if constexpr (std::is_same_v<T, Bytes>)
                return ::value_bytes(value.data(), value.size());
            else if constexpr (std::is_same_v<T, std::shared_ptr<List>>)
                return value_list_append(::value_list(value->size()), nullptr), to_c_list(*value);
            else if constexpr (std::is_same_v<T, std::shared_ptr<HashMap>>)
                return to_c_dict(*value);
            else
                return ::value_null();
        },
        v.data);
}

}  // namespace xconn
