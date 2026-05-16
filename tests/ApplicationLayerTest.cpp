#include <gtest/gtest.h>
#include <thread>
#include "ApplicationLayer.h"
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

// Full stack pair sharing one socket pair
struct AppPair {
    SocketPhysicalLayer phy_a;
    SocketPhysicalLayer phy_b;
    FramingLayer        framing_a{phy_a};
    FramingLayer        framing_b{phy_b};
    SessionLayer        session_a{framing_a};
    SessionLayer        session_b{framing_b};
    ApplicationLayer    app_a{session_a};
    ApplicationLayer    app_b{session_b};

    AppPair()
        : phy_a{}
        , phy_b(SocketPhysicalLayer::from_fd(phy_a.peer_fd()))
    {}
};

// --- Registered command is dispatched and returns correct response ---
TEST(ApplicationLayerTest, DispatchKnownCommand) {
    AppPair p;

    // Register a handler on the receiver side: echoes payload back
    p.app_b.register_command(0x01, [](const std::vector<uint8_t>& payload) {
        return payload;
    });

    std::vector<uint8_t> response;
    std::thread t([&] {
        response = p.app_b.receive_and_dispatch(500ms);
    });

    p.app_a.send(0x01, {0xAA, 0xBB});
    t.join();

    EXPECT_EQ(response, (std::vector<uint8_t>{0xAA, 0xBB}));
}

// --- Unknown msg_id returns UNKNOWN_CMD_ID byte ---
TEST(ApplicationLayerTest, UnknownCommandReturnsError) {
    AppPair p;
    // No handler registered for 0x42

    std::vector<uint8_t> response;
    std::thread t([&] {
        response = p.app_b.receive_and_dispatch(500ms);
    });

    p.app_a.send(0x42, {0x01});
    t.join();

    ASSERT_EQ(response.size(), 1u);
    EXPECT_EQ(response[0], ApplicationLayer::UNKNOWN_CMD_ID);
}

// --- Oversized payload is rejected at send ---
TEST(ApplicationLayerTest, OversizedPayloadRejected) {
    AppPair p;
    std::vector<uint8_t> big(ApplicationLayer::MAX_PAYLOAD_SIZE + 1, 0xFF);
    EXPECT_FALSE(p.app_a.send(0x01, big));
}

// --- Multiple commands can be registered and dispatched independently ---
TEST(ApplicationLayerTest, MultipleCommandsDispatched) {
    AppPair p;

    p.app_b.register_command(0x01, [](const std::vector<uint8_t>&) {
        return std::vector<uint8_t>{0x01};
    });
    p.app_b.register_command(0x02, [](const std::vector<uint8_t>&) {
        return std::vector<uint8_t>{0x02};
    });

    std::vector<uint8_t> r1, r2;
    std::thread t([&] {
        r1 = p.app_b.receive_and_dispatch(500ms);
        r2 = p.app_b.receive_and_dispatch(500ms);
    });

    p.app_a.send(0x01, {});
    p.app_a.send(0x02, {});
    t.join();

    EXPECT_EQ(r1, (std::vector<uint8_t>{0x01}));
    EXPECT_EQ(r2, (std::vector<uint8_t>{0x02}));
}

// --- receive_and_dispatch returns empty on timeout ---
TEST(ApplicationLayerTest, TimeoutReturnsEmpty) {
    AppPair p;
    auto result = p.app_a.receive_and_dispatch(100ms);
    EXPECT_TRUE(result.empty());
}
