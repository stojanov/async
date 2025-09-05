#pragma once

#include <async/io/base.h>
#include <async/io/pal/io_context.h>
#include <async/io/pal/io_handle.h>
#include <async/io/pal/io_op.h>
#include <async/io/pal/io_type.h>
#include <async/pch.h>

namespace async::internal {
class io_context_thread {
  public:
    io_context_thread(s_ptr<io::pal::io_context> io_ctx,
                      io::pal::io_context::notify_callback &&notify_cb);

    void attach_handle(io::pal::io_handle &handle, io::pal::io_type type);

    void work();

    void signal_shutdown();

  private:
    s_ptr<io::pal::io_context> _io_context;
};
} // namespace async::internal
