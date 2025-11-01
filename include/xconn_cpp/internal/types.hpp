#pragma once

#include <wampproto.h>

#include "xconn_cpp/types.hpp"

namespace xconn {

::Value* to_c_value(const Value& val);
::Value* to_c_list(const List& list);
::Value* to_c_dict(const Dict& map);

::List* vector_to_list(const List& list);
::Dict* unordered_map_to_dict(const Dict& dict);

Value from_c_value(const ::Value* v);
List from_c_list(const ::List* c_list);
Dict from_c_dict(const ::Dict* c_dict);

}  // namespace xconn
