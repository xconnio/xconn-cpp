#include "xconn_cpp/session.h"

#include <iostream>

Session::CallRequest::CallRequest(std::string uri) : uri(std::move(uri)) {}

Session::CallRequest& Session::CallRequest::Arg(const ArgType& arg) {
    args.push_back(arg);
    return *this;
}

Session::CallRequest& Session::CallRequest::Kwarg(const std::string& key, const ArgType& value) {
    kwargs[key] = value;
    return *this;
}

Session::CallRequest& Session::CallRequest::Option(const std::string& key, const ArgType& value) {
    options[key] = value;
    return *this;
}

void Session::CallRequest::Do() const {
    std::cout << "Calling URI: " << uri << "\n";

    std::cout << "Positional args:\n";
    for (const auto& arg : args) {
        std::visit(
            [](auto&& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    std::cout << "bytes: ";
                    for (auto b : value) std::cout << +b << " ";
                    std::cout << "\n";
                } else {
                    std::cout << value << "\n";
                }
            },
            arg);
    }

    std::cout << "Keyword args:\n";
    for (const auto& [key, value] : kwargs) {
        std::cout << key << " = ";
        std::visit(
            [](auto&& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    std::cout << "bytes: ";
                    for (auto b : val) std::cout << +b << " ";
                    std::cout << "\n";
                } else {
                    std::cout << val << "\n";
                }
            },
            value);
    }

    std::cout << "Network call complete!\n";
}

Session::CallRequest Session::Call(const std::string& uri) { return CallRequest(uri); }
