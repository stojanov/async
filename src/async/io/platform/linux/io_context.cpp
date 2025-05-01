#include <async/io/platform/linux/io_context.h>
#include <async/io/platform/linux/io_handle.h>

#include <spdlog/spdlog.h>
#include <sys/epoll.h>

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
    // while is running and stuff
    while (true) {
        int n =
            epoll_wait(_epfd, _epoll_events.data(), _epoll_events.size(), -1);

        for (int i = 0; i < n; i++) {
            auto &e = _epoll_events[i];
            epoll_ctl(_epfd, EPOLL_CTL_DEL, e.data.fd, &e);
            auto handle = io_handle(e.data.fd);
            _notify_cb(handle);
        }
    }
}

} // namespace async::io::lin
