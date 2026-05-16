#include "ApplicationLayer.h"

namespace proto {

ApplicationLayer::ApplicationLayer(SessionLayer& session)
    : session_(session) {}

void ApplicationLayer::register_command(uint8_t msg_id, CommandHandler handler) {
    registry_[msg_id] = std::move(handler);
}

bool ApplicationLayer::send(uint8_t msg_id, const std::vector<uint8_t>& payload) {
    if (payload.size() > MAX_PAYLOAD_SIZE) return false;
    return session_.send(msg_id, payload);
}

std::vector<uint8_t> ApplicationLayer::receive_and_dispatch(
        std::chrono::milliseconds timeout) {

    auto frame = session_.receive(timeout);
    if (!frame) return {};

    if (frame->payload.size() > MAX_PAYLOAD_SIZE) return {};

    auto it = registry_.find(frame->msg_id);
    if (it == registry_.end()) {
        // Unknown command — return a single-byte error response
        return {UNKNOWN_CMD_ID};
    }

    return it->second(frame->payload);
}

} // namespace proto
