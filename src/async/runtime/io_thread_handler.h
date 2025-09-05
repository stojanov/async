#pragma once

#include <async/io/pal/io_handle.h>
#include <async/io/pal/io_op.h>
#include <async/pch.h>
#include <async/runtime/io_context_thread.h>
#include <async/runtime/runtime_core.h>

namespace async::internal {

namespace detail {
class io_op_hasher {
  public:
    std::size_t operator()(const s_ptr<io::pal::io_op> &op) const noexcept {
        return op->handle().hash_code();
    }
};
} // namespace detail

class io_thread_handler {
  public:
    io_thread_handler(runtime_core &core);

    bool submit_io_op(s_ptr<io::pal::io_op> op);

  private:
    void notify_io_ready(const io::pal::io_handle &handle);

    inline runtime_core::thread_block *find_first_io_thread() {
        // reverse linear search works better here, since worker threads are
        // first then timer/io threads
        auto tim = std::ranges::find_if(_core._threads, [](auto &block) {
            return std::holds_alternative<io_context_thread>(
                *block.thread_work);
        });

        return tim == std::end(_core._threads) ? nullptr : &(*tim);
    }

  private:
    std::map<std::size_t, s_ptr<io::pal::io_op>> _ops;
    runtime_core &_core;
    detail::io_op_hasher _op_hasher;
};
} // namespace async::internal
