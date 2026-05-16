#include <gtest/gtest.h>
#include <thread>
#include "ApplicationLayer.h"
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

// Full stack — both sides
struct FullStack {
    SocketPhysicalLayer phy_a;
    SocketPhysicalLayer phy_b;
    FramingLayer        framing_a{phy_a};
    FramingLayer        framing_b{phy_b};
    SessionLayer        session_a{framing_a};
    SessionLayer        session_b{framing_b};
    ApplicationLayer    app_a{session_a};
    ApplicationLayer    app_b{session_b};

    FullStack()
        : phy_a{}
        , phy_b(SocketPhysicalLayer::from_fd(phy_a.peer_fd()))
    {}
};

// --- Single round-trip through all layers ---
TEST(IntegrationTest, SingleRoundTrip) {
    FullStack s;

    s.app_b.register_command(0x01, [](const std::vector<uint8_t>& p) {
        return p; // echo
    });

    std::vector<uint8_t> response;
    std::thread t([&] {
        response = s.app_b.receive_and_dispatch(500ms);
    });

    bool sent = s.app_a.send(0x01, {0x11, 0x22, 0x33});
    t.join();

    EXPECT_TRUE(sent);
    EXPECT_EQ(response, (std::vector<uint8_t>{0x11, 0x22, 0x33}));
}

// --- Payload containing framing special bytes survives full stack ---
TEST(IntegrationTest, SpecialBytesInPayload) {
    FullStack s;

    s.app_b.register_command(0x02, [](const std::vector<uint8_t>& p) {
        return p;
    });

    // These bytes would corrupt framing if byte-stuffing is broken
    std::vector<uint8_t> tricky = {0x7E, 0x7F, 0x7D, 0x00, 0xFF};

    std::vector<uint8_t> response;
    std::thread t([&] {
        response = s.app_b.receive_and_dispatch(500ms);
    });

    s.app_a.send(0x02, tricky);
    t.join();

    EXPECT_EQ(response, tricky);
}

// --- Multiple sequential messages all arrive correctly ---
TEST(IntegrationTest, MultipleSequentialMessages) {
    FullStack s;

    s.app_b.register_command(0x03, [](const std::vector<uint8_t>& p) {
        return p;
    });

    const int NUM_MESSAGES = 5;
    std::vector<std::vector<uint8_t>> responses(NUM_MESSAGES);

    std::thread t([&] {
        for (int i = 0; i < NUM_MESSAGES; ++i)
            responses[i] = s.app_b.receive_and_dispatch(500ms);
    });

    for (int i = 0; i < NUM_MESSAGES; ++i)
        s.app_a.send(0x03, {static_cast<uint8_t>(i)});

    t.join();

    for (int i = 0; i < NUM_MESSAGES; ++i)
        EXPECT_EQ(responses[i], (std::vector<uint8_t>{static_cast<uint8_t>(i)}));
}

// --- Max-size payload round-trips without corruption ---
TEST(IntegrationTest, MaxSizePayload) {
    FullStack s;

    s.app_b.register_command(0x04, [](const std::vector<uint8_t>& p) {
        return p;
    });

    std::vector<uint8_t> big(ApplicationLayer::MAX_PAYLOAD_SIZE);
    for (size_t i = 0; i < big.size(); ++i)
        big[i] = static_cast<uint8_t>(i & 0xFF);

    std::vector<uint8_t> response;
    std::thread t([&] {
        response = s.app_b.receive_and_dispatch(500ms);
    });

    s.app_a.send(0x04, big);
    t.join();

    EXPECT_EQ(response, big);
}

// --- Unknown command propagates through full stack ---
TEST(IntegrationTest, UnknownCommandFullStack) {
    FullStack s;
    // No handler registered

    std::vector<uint8_t> response;
    std::thread t([&] {
        response = s.app_b.receive_and_dispatch(500ms);
    });

    s.app_a.send(0x99, {0x01, 0x02});
    t.join();

    ASSERT_EQ(response.size(), 1u);
    EXPECT_EQ(response[0], ApplicationLayer::UNKNOWN_CMD_ID);
}
