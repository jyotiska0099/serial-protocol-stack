#include <gtest/gtest.h>
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

TEST(PhysicalLayerTest, SendAndReceive) {
    SocketPhysicalLayer phy;

    uint8_t tx[] = {0x01, 0x02, 0x03};
    ASSERT_TRUE(phy.send(tx, sizeof(tx)));

    uint8_t rx[3] = {};
    ssize_t n = ::read(phy.peer_fd(), rx, sizeof(rx));

    ASSERT_EQ(n, 3);
    EXPECT_EQ(rx[0], 0x01);
    EXPECT_EQ(rx[1], 0x02);
    EXPECT_EQ(rx[2], 0x03);
}

TEST(PhysicalLayerTest, ReceiveTimesOut) {
    SocketPhysicalLayer phy;

    uint8_t buf[16];
    auto start = std::chrono::steady_clock::now();
    size_t n = phy.receive(buf, sizeof(buf), 100ms);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(n, 0);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 80);
}

TEST(PhysicalLayerTest, AvailableReflectsWaitingBytes) {
    SocketPhysicalLayer phy;

    EXPECT_EQ(phy.available(), 0u);

    uint8_t tx[] = {0xAA, 0xBB};
    ::write(phy.peer_fd(), tx, sizeof(tx));
    usleep(5000);

    EXPECT_EQ(phy.available(), 2u);
}

TEST(PhysicalLayerTest, LargePayload) {
    SocketPhysicalLayer phy;

    std::vector<uint8_t> tx(512);
    for (size_t i = 0; i < tx.size(); ++i) tx[i] = static_cast<uint8_t>(i & 0xFF);

    ASSERT_TRUE(phy.send(tx.data(), tx.size()));

    std::vector<uint8_t> rx(512, 0);
    size_t total = 0;
    while (total < 512) {
        size_t n = ::read(phy.peer_fd(), rx.data() + total, 512 - total);
        if (n <= 0) break;
        total += n;
    }

    ASSERT_EQ(total, 512u);
    EXPECT_EQ(tx, rx);
}
