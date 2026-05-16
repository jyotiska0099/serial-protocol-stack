#include <gtest/gtest.h>
#include "FramingLayer.h"
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

TEST(FramingLayerTest, SimpleRoundTrip) {
    SocketPhysicalLayer phy;
    FramingLayer layer(phy);

    std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
    ASSERT_TRUE(layer.send(payload));

    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ASSERT_GT(n, 0);

    EXPECT_EQ(raw[0], FramingLayer::START_BYTE);
    EXPECT_EQ(raw[n - 1], FramingLayer::END_BYTE);
}

TEST(FramingLayerTest, ByteStuffingSpecialBytes) {
    SocketPhysicalLayer phy;
    FramingLayer layer(phy);

    std::vector<uint8_t> payload = {
        FramingLayer::START_BYTE,
        FramingLayer::END_BYTE,
        FramingLayer::ESC_BYTE,
        0x42
    };
    ASSERT_TRUE(layer.send(payload));

    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ASSERT_GT(n, 0);

    EXPECT_GT(n, static_cast<ssize_t>(payload.size() + 3));
}

TEST(FramingLayerTest, ReceiveRoundTrip) {
    SocketPhysicalLayer phy;
    FramingLayer layer(phy);

    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_TRUE(layer.send(payload));

    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ::write(phy.peer_fd(), raw, n);

    auto result = layer.receive(200ms);
    EXPECT_EQ(result, payload);
}

TEST(FramingLayerTest, ReceiveTimesOutOnNoData) {
    SocketPhysicalLayer phy;
    FramingLayer layer(phy);

    auto result = layer.receive(100ms);
    EXPECT_TRUE(result.empty());
}

TEST(FramingLayerTest, UnstuffsCorrectly) {
    SocketPhysicalLayer phy;
    FramingLayer layer(phy);

    std::vector<uint8_t> payload = {0x7E, 0x7F, 0x7D, 0x01, 0x02};
    ASSERT_TRUE(layer.send(payload));

    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ::write(phy.peer_fd(), raw, n);

    auto result = layer.receive(200ms);
    EXPECT_EQ(result, payload);
}
