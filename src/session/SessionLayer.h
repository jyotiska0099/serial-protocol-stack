#pragma once
#include "FramingLayer.h"
#include "CrcLayer.h"
#include <optional>
#include <cstdint>
#include <vector>
#include <chrono>

namespace proto {

enum class FrameType : uint8_t {
    DATA = 0x01,
    ACK  = 0x02,
    NACK = 0x03
};

struct SessionFrame {
    uint8_t  seq;
    FrameType type;
    uint8_t  msg_id;
    std::vector<uint8_t> payload;
};

class SessionLayer {
public:
    static constexpr int    MAX_RETRIES     = 3;
    static constexpr auto   ACK_TIMEOUT     = std::chrono::milliseconds(200);

    explicit SessionLayer(FramingLayer& framing);

    // Send payload with given msg_id. Retransmits on NACK/timeout.
    // Returns true if ACKed, false if retries exhausted.
    bool send(uint8_t msg_id, const std::vector<uint8_t>& payload);

    // Receive one DATA frame, send ACK, return its contents.
    // Returns empty optional on timeout.
    std::optional<SessionFrame> receive(std::chrono::milliseconds timeout);

private:
    FramingLayer& framing_;
    CrcLayer      crc_;
    uint8_t       seq_{0};

    std::vector<uint8_t> build_frame(uint8_t seq, FrameType type,
                                      uint8_t msg_id,
                                      const std::vector<uint8_t>& payload);

    std::optional<SessionFrame> parse_frame(const std::vector<uint8_t>& raw);

    void send_ack (uint8_t seq);
    void send_nack(uint8_t seq);
};

} // namespace proto
