#pragma once
#include "IPhysicalLayer.h"

namespace proto {

class SocketPhysicalLayer : public IPhysicalLayer {
public:
    // Creates a socketpair. fd_index=0 or 1 picks which end this instance owns.
    explicit SocketPhysicalLayer(int fd_index = 0);
    ~SocketPhysicalLayer() override;

    // Returns the other end's fd so a second instance can wrap it.
    int peer_fd() const { return fds_[1 - fd_index_]; }

    bool send(const uint8_t* data, size_t len) override;
    size_t receive(uint8_t* buf, size_t len,
                   std::chrono::milliseconds timeout) override;
    size_t available() override;

private:
    int fds_[2];     // both ends of the socketpair
    int fd_index_;   // which end this instance uses
};

} // namespace proto
