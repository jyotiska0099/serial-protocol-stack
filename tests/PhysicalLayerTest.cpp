#include <gtest/gtest.h>
#include "SocketPhysicalLayer.h"

using namespace proto;
using namespace std::chrono_literals;

// Helper: make two ends of a socket pair that talk to each other
struct SocketPair {
    SocketPhysicalLayer sender{0};
    // Wrap the peer fd in a second instance manually
    // We'll use sender to write, and read back on the same fd (loopback via fd[1])
};

// --- Basic send / receive ---
TEST(PhysicalLayerTest, SendAndReceive) {
    SocketPhysicalLayer phy(0);

    uint8_t tx[] = {0x01, 0x02, 0x03};
    // Write on fd[0], read back on fd[1] via peer_fd
    ASSERT_TRUE(phy.send(tx, sizeof(tx)));

    // Read from the other end using a raw read on peer_fd
    uint8_t rx[3] = {};
    ssize_t n = ::read(phy.peer_fd(), rx, sizeof(rx));

    ASSERT_EQ(n, 3);
    EXPECT_EQ(rx[0], 0x01);
    EXPECT_EQ(rx[1], 0x02);
    EXPECT_EQ(rx[2], 0x03);
}

// --- Receive times out when no data is available ---
TEST(PhysicalLayerTest, ReceiveTimesOut) {
    SocketPhysicalLayer phy(0);

    uint8_t buf[16];
    auto start = std::chrono::steady_clock::now();
    size_t n = phy.receive(buf, sizeof(buf), 100ms);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(n, 0);
    // Should have waited at least ~100ms
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 80);
}

// --- available() reflects bytes waiting ---
TEST(PhysicalLayerTest, AvailableReflectsWaitingBytes) {
    SocketPhysicalLayer phy(0);

    EXPECT_EQ(phy.available(), 0u);

    uint8_t tx[] = {0xAA, 0xBB};
    ::write(phy.peer_fd(), tx, sizeof(tx)); // write into our receive end
    usleep(5000); // give the kernel a moment

    EXPECT_EQ(phy.available(), 2u);
}

// --- Large payload round-trip ---
TEST(PhysicalLayerTest, LargePayload) {
    SocketPhysicalLayer phy(0);

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
