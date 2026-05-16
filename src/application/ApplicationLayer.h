#pragma once
#include "SessionLayer.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace proto {

// A command handler: takes a payload, returns a response payload.
using CommandHandler = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

class ApplicationLayer {
public:
    static constexpr size_t   MAX_PAYLOAD_SIZE = 128;
    static constexpr uint8_t  UNKNOWN_CMD_ID   = 0xFF;

    explicit ApplicationLayer(SessionLayer& session);

    // Register a handler for a given msg_id.
    void register_command(uint8_t msg_id, CommandHandler handler);

    // Send a message and wait for the session-level ACK.
    bool send(uint8_t msg_id, const std::vector<uint8_t>& payload);

    // Receive one message, dispatch to registered handler,
    // and return the handler's response. Returns empty on timeout.
    std::vector<uint8_t> receive_and_dispatch(std::chrono::milliseconds timeout);

private:
    SessionLayer& session_;
    std::unordered_map<uint8_t, CommandHandler> registry_;
};

} // namespace proto
