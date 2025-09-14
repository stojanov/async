#pragma once

#include <async/internal/io/pal/io_context.h>

#include <sys/epoll.h>

namespace async::io::lin {

class io_context : public pal::io_context {
  public:
    io_context(int event_count);

    void attach_notify_callback(
        pal::io_context::notify_callback &&callback) override;

    void attach_handle(const pal::io_handle &h, pal::io_type t) override;

    void run() override;

    void signal_shutdown() override;

  private:
    pal::io_context::notify_callback _notify_cb;
    std::vector<epoll_event> _epoll_events;
    int _epfd;
};
} // namespace async::io::lin
