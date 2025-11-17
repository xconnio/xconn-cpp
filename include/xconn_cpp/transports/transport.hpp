#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xconn {

class Transport {
   public:
    virtual ~Transport() = default;

    virtual void connect(const std::string& host, const std::string& port) = 0;
    virtual std::size_t read(uint8_t* buffer, std::size_t n) = 0;
    virtual std::size_t write(const std::vector<uint8_t>& data) = 0;
    virtual std::size_t write(const uint8_t* data, std::size_t length) = 0;

    virtual std::size_t close() = 0;

    virtual bool is_connected() const = 0;
};

}  // namespace xconn
