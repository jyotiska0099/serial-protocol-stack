#include "SocketPhysicalLayer.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>

namespace proto {

SocketPhysicalLayer::SocketPhysicalLayer(int fd_index)
    : fd_index_(fd_index)
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds_) < 0)
        throw std::runtime_error("socketpair failed");
}

SocketPhysicalLayer::~SocketPhysicalLayer() {
    close(fds_[0]);
    close(fds_[1]);
}

bool SocketPhysicalLayer::send(const uint8_t* data, size_t len) {
    return ::write(fds_[fd_index_], data, len) == static_cast<ssize_t>(len);
}

size_t SocketPhysicalLayer::receive(uint8_t* buf, size_t len,
                                     std::chrono::milliseconds timeout) {
    // Use select() to wait up to `timeout` ms before reading
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int fd = fds_[fd_index_];
    FD_SET(fd, &read_fds);

    timeval tv{};
    tv.tv_sec  = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;

    int ready = select(fd + 1, &read_fds, nullptr, nullptr, &tv);
    if (ready <= 0) return 0; // timeout or error

    ssize_t n = ::read(fd, buf, len);
    return n > 0 ? static_cast<size_t>(n) : 0;
}

size_t SocketPhysicalLayer::available() {
    int bytes = 0;
    ioctl(fds_[fd_index_], FIONREAD, &bytes);
    return static_cast<size_t>(bytes);
}

} // namespace proto
