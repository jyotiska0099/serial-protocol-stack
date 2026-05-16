#include <gtest/gtest.h>
#include "CrcLayer.h"

using namespace proto;

// --- Known vector ---
// CRC-16/CCITT of ASCII "123456789" is 0x29B1 — a standard test vector.
TEST(CrcLayerTest, KnownVector) {
    CrcLayer crc;
    const uint8_t data[] = "123456789";
    uint16_t result = crc.compute(data, 9);
    EXPECT_EQ(result, 0x29B1);
}

// --- Encode appends 2 bytes ---
TEST(CrcLayerTest, EncodeAppendsTwoBytes) {
    CrcLayer crc;
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
    auto encoded = crc.encode(payload);
    EXPECT_EQ(encoded.size(), payload.size() + 2);
}

// --- Decode recovers original payload ---
TEST(CrcLayerTest, DecodeRoundTrip) {
    CrcLayer crc;
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    auto encoded = crc.encode(payload);
    auto decoded = crc.decode(encoded);
    EXPECT_EQ(decoded, payload);
}

// --- Single bit flip is detected ---
TEST(CrcLayerTest, SingleBitErrorDetected) {
    CrcLayer crc;
    std::vector<uint8_t> payload = {0xAA, 0xBB, 0xCC};
    auto encoded = crc.encode(payload);

    encoded[1] ^= 0x01; // flip one bit in the payload

    auto decoded = crc.decode(encoded);
    EXPECT_TRUE(decoded.empty());
}

// --- Burst error (corrupted CRC bytes) is detected ---
TEST(CrcLayerTest, CorruptCrcDetected) {
    CrcLayer crc;
    std::vector<uint8_t> payload = {0x11, 0x22, 0x33};
    auto encoded = crc.encode(payload);

    // Corrupt both CRC bytes
    encoded[encoded.size() - 2] ^= 0xFF;
    encoded[encoded.size() - 1] ^= 0xFF;

    auto decoded = crc.decode(encoded);
    EXPECT_TRUE(decoded.empty());
}

// --- Empty payload ---
TEST(CrcLayerTest, EmptyPayloadRoundTrip) {
    CrcLayer crc;
    std::vector<uint8_t> payload = {};
    auto encoded = crc.encode(payload);
    auto decoded = crc.decode(encoded);
    EXPECT_EQ(decoded, payload);
}

// --- Too-short input to decode ---
TEST(CrcLayerTest, DecodeTooShortReturnsEmpty) {
    CrcLayer crc;
    std::vector<uint8_t> bad = {0x01}; // only 1 byte, need at least 2
    EXPECT_TRUE(crc.decode(bad).empty());
}
