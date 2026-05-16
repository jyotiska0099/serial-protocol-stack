#pragma once
#include "IPhysicalLayer.h"
#include <vector>
#include <cstdint>

namespace proto {

class FramingLayer {
public:
    explicit FramingLayer(IPhysicalLayer& phy);

    // Wraps payload in a frame and sends it via the physical layer.
    bool send(const std::vector<uint8_t>& payload);

    // Reads from physical layer and returns the decoded payload.
    // Returns empty vector on timeout or framing error.
    std::vector<uint8_t> receive(std::chrono::milliseconds timeout);

    // --- Constants ---
    static constexpr uint8_t START_BYTE  = 0x7E;
    static constexpr uint8_t END_BYTE    = 0x7F;
    static constexpr uint8_t ESC_BYTE    = 0x7D;
    static constexpr uint8_t ESC_XOR     = 0x20;

private:
    IPhysicalLayer& phy_;

    std::vector<uint8_t> stuff(const std::vector<uint8_t>& data);
    std::vector<uint8_t> unstuff(const std::vector<uint8_t>& data);
};

} // namespace proto
