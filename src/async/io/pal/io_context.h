#pragma once

#include <async/io/pal/io_handle.h>
#include <async/io/pal/io_type.h>

namespace async::io::pal {
class io_context {
  public:
    using notify_callback = std::function<void(const io::pal::io_handle &)>;

  public:
    virtual void attach_notify_callback(notify_callback &&callback) = 0;

    virtual void attach_handle(const io_handle &h, io_type t) = 0;

    virtual void run() = 0;
};
} // namespace async::io::pal
