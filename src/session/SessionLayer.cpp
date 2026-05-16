#include "SessionLayer.h"
#include <optional>

namespace proto {

SessionLayer::SessionLayer(FramingLayer& framing) : framing_(framing) {}

std::vector<uint8_t> SessionLayer::build_frame(uint8_t seq, FrameType type,
                                                 uint8_t msg_id,
                                                 const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> raw;
    raw.push_back(seq);
    raw.push_back(static_cast<uint8_t>(type));
    raw.push_back(msg_id);
    raw.insert(raw.end(), payload.begin(), payload.end());
    return crc_.encode(raw);
}

std::optional<SessionFrame> SessionLayer::parse_frame(const std::vector<uint8_t>& raw) {
    // CRC decode first
    auto decoded = crc_.decode(raw);
    if (decoded.size() < 3) return std::nullopt; // seq + type + msg_id minimum

    SessionFrame f;
    f.seq     = decoded[0];
    f.type    = static_cast<FrameType>(decoded[1]);
    f.msg_id  = decoded[2];
    f.payload = std::vector<uint8_t>(decoded.begin() + 3, decoded.end());
    return f;
}

void SessionLayer::send_ack(uint8_t seq) {
    auto frame = build_frame(seq, FrameType::ACK, 0, {});
    framing_.send(frame);
}

void SessionLayer::send_nack(uint8_t seq) {
    auto frame = build_frame(seq, FrameType::NACK, 0, {});
    framing_.send(frame);
}

bool SessionLayer::send(uint8_t msg_id, const std::vector<uint8_t>& payload) {
    uint8_t seq = seq_++;
    auto frame = build_frame(seq, FrameType::DATA, msg_id, payload);

    for (int attempt = 0; attempt <= MAX_RETRIES; ++attempt) {
        framing_.send(frame);

        // Wait for ACK or NACK
        auto raw = framing_.receive(ACK_TIMEOUT);
        if (raw.empty()) continue; // timeout → retransmit

        auto response = parse_frame(raw);
        if (!response) continue; // corrupt response → retransmit

        if (response->seq != seq) continue; // wrong seq → retransmit

        if (response->type == FrameType::ACK)  return true;
        if (response->type == FrameType::NACK) continue; // retransmit
    }
    return false; // retries exhausted
}

std::optional<SessionFrame> SessionLayer::receive(std::chrono::milliseconds timeout) {
    auto raw = framing_.receive(timeout);
    if (raw.empty()) return std::nullopt;

    auto frame = parse_frame(raw);
    if (!frame) {
        // Corrupt frame — send NACK with seq=0 (best effort)
        send_nack(0);
        return std::nullopt;
    }

    if (frame->type != FrameType::DATA) return std::nullopt;

    send_ack(frame->seq);
    return frame;
}

} // namespace proto
