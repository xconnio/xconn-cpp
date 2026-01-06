#pragma once
#include <cstdint>
#include <system_error>

#include "transport.hpp"

#include <asio.hpp>

namespace xconn {

class UnixTransport : public Transport {
   public:
    explicit UnixTransport(asio::io_context& io) : socket_(io) {}

    void connect(const std::string& path, const std::string&) override {
        socket_.connect(asio::local::stream_protocol::endpoint(path));
    }

    std::size_t read(uint8_t* buffer, std::size_t length) override {
        std::error_code ec;
        std::size_t n = socket_.read_some(asio::buffer(buffer, length), ec);
        if (ec) throw std::system_error(ec);
        return n;
    }

    std::size_t write(const uint8_t* data, std::size_t length) override {
        std::error_code ec;
        std::size_t n = asio::write(socket_, asio::buffer(data, length), ec);
        if (ec) throw std::system_error(ec);
        return n;
    }

    std::size_t write(const std::vector<uint8_t>& data) override {
        std::error_code ec;
        std::size_t n = asio::write(socket_, asio::buffer(data), ec);
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
            ec = socket_.shutdown(asio::local::stream_protocol::socket::shutdown_send, ec);
            if (ec && ec != asio::error::not_connected) throw std::system_error(ec);
        }
        return 0;
    }

    bool is_connected() const override { return socket_.is_open(); }

   private:
    asio::local::stream_protocol::socket socket_;
};

}  // namespace xconn
