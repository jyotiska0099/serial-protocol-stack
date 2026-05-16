#include "CrcLayer.h"
#include <stdexcept>

namespace proto {

CrcLayer::CrcLayer() {
    build_table();
}

// Build the 256-entry lookup table from scratch.
// For each possible byte value, compute its CRC contribution.
void CrcLayer::build_table() {
    for (int i = 0; i < 256; ++i) {
        uint16_t crc = static_cast<uint16_t>(i << 8);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000)
                crc = static_cast<uint16_t>((crc << 1) ^ POLYNOMIAL);
            else
                crc = static_cast<uint16_t>(crc << 1);
        }
        table_[i] = crc;
    }
}

// Table-driven CRC: one XOR + one table lookup per byte.
uint16_t CrcLayer::compute(const uint8_t* data, size_t len) {
    uint16_t crc = INIT_VALUE;
    for (size_t i = 0; i < len; ++i) {
        uint8_t index = static_cast<uint8_t>((crc >> 8) ^ data[i]);
        crc = static_cast<uint16_t>((crc << 8) ^ table_[index]);
    }
    return crc;
}

std::vector<uint8_t> CrcLayer::encode(const std::vector<uint8_t>& payload) {
    uint16_t crc = compute(payload.data(), payload.size());

    std::vector<uint8_t> out = payload;
    out.push_back(static_cast<uint8_t>(crc >> 8));   // CRC_HI
    out.push_back(static_cast<uint8_t>(crc & 0xFF)); // CRC_LO
    return out;
}

std::vector<uint8_t> CrcLayer::decode(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return {};

    // Recompute CRC over everything except the last 2 bytes
    size_t payload_len = data.size() - 2;
    uint16_t computed = compute(data.data(), payload_len);

    // Extract the received CRC
    uint16_t received = static_cast<uint16_t>(
        (static_cast<uint16_t>(data[payload_len]) << 8) | data[payload_len + 1]
    );

    if (computed != received) return {};

    return std::vector<uint8_t>(data.begin(), data.begin() + payload_len);
}

} // namespace proto
