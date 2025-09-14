#include <async/internal/io/pal/io_context.h>
#include <async/internal/io/pal/io_handle.h>
#include <async/internal/runtime/io_context_thread.h>

namespace async::internal {

io_context_thread::io_context_thread(
    s_ptr<io::pal::io_context> io_ctx,
    io::pal::io_context::notify_callback &&notify_cb)
    : _io_context(io_ctx) {
    _io_context->attach_notify_callback(std::move(notify_cb));
}

void io_context_thread::attach_handle(io::pal::io_handle &handle,
                                      io::pal::io_type type) {
    _io_context->attach_handle(handle, type);
}

void io_context_thread::work() { _io_context->run(); }

void io_context_thread::signal_shutdown() { _io_context->signal_shutdown(); }

} // namespace async::internal
