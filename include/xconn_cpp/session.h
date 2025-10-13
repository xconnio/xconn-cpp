#ifndef XCONN_CPP_SESSION_H
#define XCONN_CPP_SESSION_H

#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class Session {
   public:
    using ArgType = std::variant<int, double, std::string, bool, std::vector<uint8_t> >;

    class CallRequest {
       public:
        explicit CallRequest(std::string uri);

        CallRequest& Arg(const ArgType& arg);
        CallRequest& Kwarg(const std::string& key, const ArgType& value);
        CallRequest& Option(const std::string& key, const ArgType& value);

        void Do() const;

       private:
        std::string uri;
        std::vector<ArgType> args;
        std::unordered_map<std::string, ArgType> kwargs;
        std::unordered_map<std::string, ArgType> options;
    };

    CallRequest Call(const std::string& uri);
};

#endif  // XCONN_CPP_SESSION_H
