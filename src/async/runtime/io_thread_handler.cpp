#include <async/io/base.h>
#include <async/runtime/io_thread_handler.h>
#include <memory>

namespace async::runtime {
io_thread_handler::io_thread_handler(runtime_core &core) : _core(core) {}

// TODO: potential problems with this
// same handle can be attached to different threads
// releasing/notifying can cause issuses/undef behaviour
// FIX!
bool io_thread_handler::submit_io_op(s_ptr<io::pal::io_op> op) {
    io_context_thread *th = nullptr;

    if (auto i = _ops.find(_op_hasher(op)); i != std::end(_ops)) {
        return false;
    }

    if (auto *th_block = find_first_io_thread()) {
        th = &std::get<io_context_thread>(*th_block->thread_work);
    } else {
        // TODO: REFINE THIS!
        const auto ctx = std::make_shared<io::io_context>(30000);

        auto notify_cb = [&](const io::pal::io_handle &handle) {
            notify_io_ready(handle);
        };

        // const auto &new_block = _core.spawn_new<io_context_thread>(
        //     std::dynamic_pointer_cast<io::pal::io_context>(ctx),
        //     std::move(notify_cb));

        const auto &new_block =
            _core.spawn_new<io_context_thread>(ctx, notify_cb);

        th = &std::get<io_context_thread>(*new_block.thread_work);
    }

    th->attach_handle(op->handle(), op->type());

    _ops.insert(std::pair{_op_hasher(op), op});

    return true;
}

void io_thread_handler::notify_io_ready(const io::pal::io_handle &handle) {
    if (auto i = _ops.find(handle.hash_code()); i != std::end(_ops)) {
        i->second->do_op();
        _ops.erase(i);
    }
}

} // namespace async::runtime
