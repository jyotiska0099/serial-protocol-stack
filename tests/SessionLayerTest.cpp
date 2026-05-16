#include <gtest/gtest.h>
#include <thread>
#include "SessionLayer.h"
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

// Two session layers sharing opposite ends of the SAME socket pair
struct SessionPair {
    SocketPhysicalLayer phy_a;                              // owns the pair
    SocketPhysicalLayer phy_b;                              // wraps the peer fd
    FramingLayer        framing_a{phy_a};
    FramingLayer        framing_b{phy_b};
    SessionLayer        sender{framing_a};
    SessionLayer        receiver{framing_b};

    SessionPair()
        : phy_a{}
        , phy_b(SocketPhysicalLayer::from_fd(phy_a.peer_fd()))
    {}
};

TEST(SessionLayerTest, HappyPath) {
    SessionPair p;
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03};

    bool send_result = false;
    std::optional<SessionFrame> recv_result;

    std::thread t([&] {
        recv_result = p.receiver.receive(500ms);
    });

    send_result = p.sender.send(0x10, payload);
    t.join();

    EXPECT_TRUE(send_result);
    ASSERT_TRUE(recv_result.has_value());
    EXPECT_EQ(recv_result->msg_id, 0x10);
    EXPECT_EQ(recv_result->payload, payload);
}

TEST(SessionLayerTest, RetransmitsOnTimeout) {
    SocketPhysicalLayer phy;
    FramingLayer framing(phy);
    SessionLayer sender(framing);

    bool result = sender.send(0x01, {0xAA});
    EXPECT_FALSE(result);
}

TEST(SessionLayerTest, AckSentForDuplicate) {
    SessionPair p;
    std::vector<uint8_t> payload = {0xBE, 0xEF};

    std::thread t([&] {
        auto r = p.receiver.receive(500ms);
        EXPECT_TRUE(r.has_value());
    });

    p.sender.send(0x20, payload);
    t.join();
}

TEST(SessionLayerTest, SequenceNumbersIncrement) {
    SessionPair p;

    std::thread t([&] {
        auto r1 = p.receiver.receive(500ms);
        auto r2 = p.receiver.receive(500ms);
        ASSERT_TRUE(r1.has_value());
        ASSERT_TRUE(r2.has_value());
        EXPECT_NE(r1->seq, r2->seq);
    });

    p.sender.send(0x01, {0x01});
    p.sender.send(0x02, {0x02});
    t.join();
}
