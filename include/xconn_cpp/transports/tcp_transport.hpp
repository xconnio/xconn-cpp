#pragma once
#include <cstddef>
#include <cstdint>

#include "asio/error.hpp"
#include "transport.hpp"

#include <asio.hpp>

namespace xconn {

class TcpTransport : public Transport {
   public:
    explicit TcpTransport(asio::io_context& io) : socket_(io) {}

    void connect(const std::string& host, const std::string& port) override {
        asio::ip::tcp::resolver resolver(socket_.get_executor());
        auto endpoints = resolver.resolve(host, port);
        asio::connect(socket_, endpoints);
    }

    std::size_t read(uint8_t* buffer, std::size_t length) override {
        std::error_code ec;
        std::size_t n = socket_.read_some(asio::buffer(buffer, length), ec);
        if (ec) throw std::system_error(ec);
        return n;
    }

    std::size_t write(const std::vector<uint8_t>& data) override {
        std::error_code ec;
        std::size_t n = asio::write(socket_, asio::buffer(data), ec);
        if (ec) throw std::system_error(ec);
        return n;
    }

    std::size_t write(const uint8_t* data, std::size_t length) override {
        std::error_code ec;
        std::size_t n = asio::write(socket_, asio::buffer(data, length), ec);
        if (ec) throw std::system_error(ec);
        return n;
    }

    std::size_t close() override {
        std::error_code ec;

        if (socket_.is_open()) {
            ec = socket_.close(ec);
            if (ec && ec != asio::error::not_connected) throw std::system_error(ec);
        }

        return 0;
    }

    std::size_t shutdown() override {
        std::error_code ec;

        if (socket_.is_open()) {
            ec = socket_.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
            if (ec && ec != asio::error::not_connected) throw std::system_error(ec);
        }

        return 0;
    }

    bool is_connected() const override { return socket_.is_open(); }

   private:
    asio::ip::tcp::socket socket_;
};

}  // namespace xconn
