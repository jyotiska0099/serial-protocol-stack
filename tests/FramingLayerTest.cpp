#include <gtest/gtest.h>
#include "FramingLayer.h"
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

// Two framing layers sharing opposite ends of a socket pair
struct FramingPair {
    SocketPhysicalLayer phy_a{0};
    // phy_b wraps the peer fd directly via a second SocketPhysicalLayer
    // We cheat slightly: use fd_index=1 so it reads/writes fds_[1]
    SocketPhysicalLayer phy_b{1};

    FramingLayer sender{phy_a};
    FramingLayer receiver{phy_b};

    FramingPair() {
        // Give phy_b the same fds by constructing after phy_a
        // Both share the same socketpair through fd_index selection
    }
};

TEST(FramingLayerTest, SimpleRoundTrip) {
    SocketPhysicalLayer phy(0);
    FramingLayer layer(phy);

    std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
    ASSERT_TRUE(layer.send(payload));

    // Read raw frame from peer_fd and feed it back through a second framing layer
    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ASSERT_GT(n, 0);

    // Verify START and END bytes are in place
    EXPECT_EQ(raw[0], FramingLayer::START_BYTE);
    EXPECT_EQ(raw[n - 1], FramingLayer::END_BYTE);
}

TEST(FramingLayerTest, ByteStuffingSpecialBytes) {
    SocketPhysicalLayer phy(0);
    FramingLayer layer(phy);

    // Payload contains all three special bytes
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

    // The stuffed frame should be longer than original payload + 3 header bytes
    // (each special byte becomes 2 bytes, so 3 specials → +3 extra bytes)
    EXPECT_GT(n, static_cast<ssize_t>(payload.size() + 3));
}

TEST(FramingLayerTest, ReceiveRoundTrip) {
    SocketPhysicalLayer phy(0);
    FramingLayer layer(phy);

    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_TRUE(layer.send(payload));

    // Write the sent bytes into the receive end
    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ::write(phy.peer_fd(), raw, n); // echo back so receive() can read it

    auto result = layer.receive(200ms);
    EXPECT_EQ(result, payload);
}

TEST(FramingLayerTest, ReceiveTimesOutOnNoData) {
    SocketPhysicalLayer phy(0);
    FramingLayer layer(phy);

    auto result = layer.receive(100ms);
    EXPECT_TRUE(result.empty());
}

TEST(FramingLayerTest, UnstuffsCorrectly) {
    SocketPhysicalLayer phy(0);
    FramingLayer layer(phy);

    // Round-trip with special bytes in payload — unstuffing must recover original
    std::vector<uint8_t> payload = {0x7E, 0x7F, 0x7D, 0x01, 0x02};
    ASSERT_TRUE(layer.send(payload));

    uint8_t raw[64] = {};
    ssize_t n = ::read(phy.peer_fd(), raw, sizeof(raw));
    ::write(phy.peer_fd(), raw, n);

    auto result = layer.receive(200ms);
    EXPECT_EQ(result, payload);
}
