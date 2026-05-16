#include "SocketPhysicalLayer.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>

namespace proto {

SocketPhysicalLayer::SocketPhysicalLayer() : owns_pair_(true) {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        throw std::runtime_error("socketpair failed");
    my_fd_   = fds[0];
    pair_fd_ = fds[1];
}

SocketPhysicalLayer::SocketPhysicalLayer(int my_fd, bool owns_pair, int pair_fd)
    : my_fd_(my_fd), pair_fd_(pair_fd), owns_pair_(owns_pair) {}

SocketPhysicalLayer SocketPhysicalLayer::from_fd(int fd) {
    return SocketPhysicalLayer(fd, false, -1);
}

SocketPhysicalLayer::~SocketPhysicalLayer() {
    if (owns_pair_) {
        close(my_fd_);
        close(pair_fd_);
    }
}

int SocketPhysicalLayer::peer_fd() const {
    return pair_fd_;
}

bool SocketPhysicalLayer::send(const uint8_t* data, size_t len) {
    return ::write(my_fd_, data, len) == static_cast<ssize_t>(len);
}

size_t SocketPhysicalLayer::receive(uint8_t* buf, size_t len,
                                     std::chrono::milliseconds timeout) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(my_fd_, &read_fds);

    timeval tv{};
    tv.tv_sec  = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;

    int ready = select(my_fd_ + 1, &read_fds, nullptr, nullptr, &tv);
    if (ready <= 0) return 0;

    ssize_t n = ::read(my_fd_, buf, len);
    return n > 0 ? static_cast<size_t>(n) : 0;
}

size_t SocketPhysicalLayer::available() {
    int bytes = 0;
    ioctl(my_fd_, FIONREAD, &bytes);
    return static_cast<size_t>(bytes);
}

} // namespace proto
