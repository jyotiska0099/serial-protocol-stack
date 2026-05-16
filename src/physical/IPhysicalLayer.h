#pragma once
#include <cstddef>
#include <cstdint>
#include <chrono>

namespace proto {

class IPhysicalLayer {
public:
    virtual ~IPhysicalLayer() = default;

    // Send len bytes from data. Returns true on success.
    virtual bool send(const uint8_t* data, size_t len) = 0;

    // Block until up to len bytes are received or timeout expires.
    // Returns the number of bytes actually read (0 = timeout).
    virtual size_t receive(uint8_t* buf, size_t len,
                           std::chrono::milliseconds timeout) = 0;

    // How many bytes are waiting to be read right now.
    virtual size_t available() = 0;
};

} // namespace proto
