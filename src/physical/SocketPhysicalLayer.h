#pragma once
#include "IPhysicalLayer.h"

namespace proto {

class SocketPhysicalLayer : public IPhysicalLayer {
public:
    // Creates a new socketpair and owns both fds.
    SocketPhysicalLayer();

    // Wraps an existing fd (non-owning — does NOT close it on destruction).
    static SocketPhysicalLayer from_fd(int fd);

    ~SocketPhysicalLayer() override;

    // Returns the peer fd so a second instance can wrap it.
    int peer_fd() const;

    bool   send(const uint8_t* data, size_t len) override;
    size_t receive(uint8_t* buf, size_t len,
                   std::chrono::milliseconds timeout) override;
    size_t available() override;

private:
    // Private constructor used by from_fd()
    SocketPhysicalLayer(int my_fd, bool owns_pair, int pair_fd);

    int  my_fd_;       // fd this instance reads/writes
    int  pair_fd_;     // the other end (-1 if non-owning)
    bool owns_pair_;   // whether we should close both fds on destruction
};

} // namespace proto
