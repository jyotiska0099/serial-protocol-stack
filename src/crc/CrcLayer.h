#pragma once
#include <vector>
#include <cstdint>

namespace proto {

class CrcLayer {
public:
    CrcLayer();

    // Appends 2-byte CRC to payload and returns the result.
    std::vector<uint8_t> encode(const std::vector<uint8_t>& payload);

    // Verifies CRC and returns the payload (without CRC bytes).
    // Returns empty vector if CRC check fails.
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data);

    // Compute CRC-16/CCITT over a buffer (public for testing).
    uint16_t compute(const uint8_t* data, size_t len);

private:
    void build_table();

    static constexpr uint16_t POLYNOMIAL = 0x1021;
    static constexpr uint16_t INIT_VALUE = 0xFFFF;

    uint16_t table_[256];
};

} // namespace proto
