#pragma once
#include <mutex>
#include <string>
#include <vector>
#include <wampproto.h>

#include "xconn_cpp/transports.hpp"
#include "xconn_cpp/types.hpp"
#include "xconn_cpp/url_parser.hpp"

#include "wampproto/value.h"

#include <asio.hpp>

class SocketTransport {
   public:
    SocketTransport(asio::io_context& io, UrlParser& parser);
    ~SocketTransport();
    static std::shared_ptr<SocketTransport> Create(std::string& url);

    bool connect(const std::string& host, const std::string& port, xconn::SerializerType_ serializer_type,
                 int max_msg_size);
    std::vector<uint8_t> read();
    Bytes read_bytes();
    bool write(Bytes& bytes);
    void close();
    bool is_connected() const;

   private:
    std::unique_ptr<Transport> transport_;
    std::mutex write_mutex_;

    bool recv_exactly(uint8_t* buffer, size_t n);
};

void run();
