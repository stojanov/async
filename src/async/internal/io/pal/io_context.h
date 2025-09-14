#pragma once

#include <async/internal/io/pal/io_handle.h>
#include <async/internal/io/pal/io_type.h>

namespace async::io::pal {
class io_context {
  public:
    using notify_callback = std::function<void(const io::pal::io_handle &)>;

  public:
    virtual void attach_notify_callback(notify_callback &&callback) = 0;

    virtual void attach_handle(const io_handle &h, io_type t) = 0;

    // This must not have a loop
    // only the implementation of the waiting event loop
    // loop is done top side
    virtual void run() = 0;

    virtual void signal_shutdown() = 0;
};
} // namespace async::io::pal
