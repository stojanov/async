#include <async/internal/io/platform/linux/io_context.h>
#include <async/internal/io/platform/linux/io_handle.h>

#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

namespace async::io::lin {
io_context::io_context(int event_count) {
    _epoll_events.resize(event_count);

    _epfd = epoll_create1(0);
}

void io_context::attach_notify_callback(
    pal::io_context::notify_callback &&callback) {
    _notify_cb = callback;
}

void io_context::attach_handle(const pal::io_handle &h, pal::io_type t) {
    int fd = std::any_cast<int>(h.native());

    uint32_t events = 0;

    switch (t) {
    case pal::io_type::IN:
        events = EPOLLIN;
        break;
    case pal::io_type::OUT:
        events = EPOLLOUT;
        break;
    case pal::io_type::HUP:
        events = EPOLLHUP;
        break;
    case pal::io_type::ERR:
        events = EPOLLERR;
        break;
    }

    struct epoll_event event;

    event.events = events;
    event.data.fd = fd;

    epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &event);
    spdlog::warn("ADDED TO WAIT EPOLL");
}

void io_context::run() {
    int n = epoll_wait(_epfd, _epoll_events.data(), _epoll_events.size(), -1);

    for (int i = 0; i < n; i++) {
        auto &e = _epoll_events[i];
        const auto handle = io_handle(e.data.fd);
        epoll_ctl(_epfd, EPOLL_CTL_DEL, e.data.fd, &e);
        _notify_cb(handle);
    }
}

void io_context::signal_shutdown() {
    int efd = eventfd(0, EFD_NONBLOCK);

    struct epoll_event epoll_e = {0};
    epoll_e.events = EPOLLIN;
    epoll_e.data.fd = efd;

    epoll_ctl(_epfd, EPOLL_CTL_ADD, efd, &epoll_e);

    internal::u64 n = 1;
    write(efd, &n, sizeof(n));
}

} // namespace async::io::lin
