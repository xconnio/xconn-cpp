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

::Value* to_c_dict(const Dict& map) {
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
            else if constexpr (std::is_same_v<T, std::shared_ptr<Dict>>)
                return to_c_dict(*value);
            else
                return ::value_null();
        },
        v.data);
}

::List* vector_to_list(const List& list) {
    ::Value* v = to_c_list(list);

    return value_as_list(v);
}
::Dict* unordered_map_to_dict(const Dict& dict) {
    ::Value* v = to_c_dict(dict);

    return value_as_dict(v);
}

Value from_c_value(const ::Value* v);

List from_c_list(const ::List* c_list) {
    List list;
    list.reserve(c_list->len);

    for (size_t i = 0; i < c_list->len; ++i) {
        ::Value* elem = c_list->items[i];
        list.push_back(from_c_value(elem));
    }
    return list;
}

void dict_interator(const char* key, ::Value* val, void* item) {
    Dict* dict = static_cast<Dict*>(item);
    (*dict)[key] = from_c_value(val);
}

Dict from_c_dict(const ::Dict* c_dict) {
    Dict dict;
    dict_iter_cb callback = dict_interator;
    dict_foreach(c_dict, callback, &dict);

    return dict;
}

Value from_c_value(const ::Value* v) {
    if (!v) return Value{};  // monostate

    switch (v->type) {
        case VALUE_NULL:
            return std::monostate{};

        case VALUE_INT:
            return static_cast<int>(v->int_val);

        case VALUE_FLOAT:
            return v->float_val;

        case VALUE_BOOL:
            return static_cast<bool>(v->bool_val);

        case VALUE_STR:
            return std::string(v->str_val ? v->str_val : "");

        case VALUE_BYTES:
            return Bytes(v->bytes_val.data, v->bytes_val.data + v->bytes_val.len);

        case VALUE_LIST:
            return std::make_shared<List>(from_c_list(v->list_val));

        case VALUE_DICT:
            return std::make_shared<Dict>(from_c_dict(v->dict_val));

        default:
            return std::monostate{};
    }
}

Result::Result(void* c_result) {
    ::Result* result = (::Result*)c_result;
    args = from_c_list(result->args);
    kwargs = from_c_dict(result->kwargs);
    details = from_c_dict(result->details);
}

Result::Result(const Invocation& invocation) {}

Invocation::Invocation(void* c_invocation) {
    ::Invocation* invocation = (::Invocation*)c_invocation;
    args = from_c_list(invocation->args);
    kwargs = from_c_dict(invocation->kwargs);
    details = from_c_dict(invocation->details);
}

Event::Event(void* c_event) {
    ::Event* event = (::Event*)c_event;
    args = from_c_list(event->args);
    kwargs = from_c_dict(event->kwargs);
    details = from_c_dict(event->details);
}

// Forward declarations (needed for mutual references)
std::ostream& operator<<(std::ostream& os, const Value& v);
std::ostream& operator<<(std::ostream& os, const List& list);
std::ostream& operator<<(std::ostream& os, const Dict& dict);

// --- Pretty-print List ---
std::ostream& operator<<(std::ostream& os, const List& list) {
    os << "[";
    bool first = true;
    for (const auto& v : list) {
        if (!first) os << ", ";
        os << v;
        first = false;
    }
    os << "]";
    return os;
}

// --- Pretty-print Dict ---
std::ostream& operator<<(std::ostream& os, const Dict& dict) {
    os << "{";
    bool first = true;
    for (const auto& [k, v] : dict) {
        if (!first) os << ", ";
        os << '"' << k << "\": " << v;
        first = false;
    }
    os << "}";
    return os;
}

// --- Pretty-print Value ---
std::ostream& operator<<(std::ostream& os, const Value& v) {
    std::visit(
        [&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                os << "null";
            } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double> || std::is_same_v<T, bool>) {
                os << val;
            } else if constexpr (std::is_same_v<T, std::string>) {
                os << '"' << val << '"';
            } else if constexpr (std::is_same_v<T, Bytes>) {
                os << "bytes[" << val.size() << "]";
            } else if constexpr (std::is_same_v<T, std::shared_ptr<List>>) {
                if (val)
                    os << *val;
                else
                    os << "null_list";
            } else if constexpr (std::is_same_v<T, std::shared_ptr<Dict>>) {
                if (val)
                    os << *val;
                else
                    os << "null_dict";
            }
        },
        v.data);
    return os;
}

}  // namespace xconn
