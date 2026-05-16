#include "FramingLayer.h"
#include <stdexcept>

namespace proto {

FramingLayer::FramingLayer(IPhysicalLayer& phy) : phy_(phy) {}

// Escape any special bytes in the payload
std::vector<uint8_t> FramingLayer::stuff(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(data.size());
    for (uint8_t b : data) {
        if (b == START_BYTE || b == END_BYTE || b == ESC_BYTE) {
            out.push_back(ESC_BYTE);
            out.push_back(b ^ ESC_XOR);
        } else {
            out.push_back(b);
        }
    }
    return out;
}

// Reverse the escaping
std::vector<uint8_t> FramingLayer::unstuff(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(data.size());
    bool escaping = false;
    for (uint8_t b : data) {
        if (escaping) {
            out.push_back(b ^ ESC_XOR);
            escaping = false;
        } else if (b == ESC_BYTE) {
            escaping = true;
        } else {
            out.push_back(b);
        }
    }
    return out;
}

bool FramingLayer::send(const std::vector<uint8_t>& payload) {
    auto stuffed = stuff(payload);

    // Build full frame: START | LEN | stuffed payload | END
    std::vector<uint8_t> frame;
    frame.reserve(3 + stuffed.size());
    frame.push_back(START_BYTE);
    frame.push_back(static_cast<uint8_t>(stuffed.size()));
    frame.insert(frame.end(), stuffed.begin(), stuffed.end());
    frame.push_back(END_BYTE);

    return phy_.send(frame.data(), frame.size());
}

std::vector<uint8_t> FramingLayer::receive(std::chrono::milliseconds timeout) {
    // Read START byte
    uint8_t b = 0;
    if (phy_.receive(&b, 1, timeout) == 0 || b != START_BYTE)
        return {};

    // Read LEN byte
    if (phy_.receive(&b, 1, timeout) == 0)
        return {};
    size_t len = b;

    // Read stuffed payload
    std::vector<uint8_t> stuffed(len);
    for (size_t i = 0; i < len; ++i) {
        if (phy_.receive(&stuffed[i], 1, timeout) == 0)
            return {};
    }

    // Read END byte
    if (phy_.receive(&b, 1, timeout) == 0 || b != END_BYTE)
        return {};

    return unstuff(stuffed);
}

} // namespace proto
