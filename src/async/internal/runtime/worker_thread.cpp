#include <async/internal/runtime/runqueue.h>
#include <async/internal/runtime/runtime_core.h>
#include <async/internal/runtime/worker_thread.h>

namespace async::internal {

// optimize all copying here
struct work_visitor {
    inline void operator()(runqueue::pending_task &task) {
        th->_core._runqueue.activate(task);
    }

    inline void operator()(coro_handle handle) {
        spdlog::warn("RESUMING WORK");
        handle.resume();
    }

    worker_thread *th;
};

// Refactor
worker_thread::worker_thread(runtime_core &core) : _core(core) {}

// consider exit case
void worker_thread::activate_pending_work() {
    // copying here, optimize
    // TODO: think about all the variant, optional, any: copy, move cost
    if (auto task = std::move(_core._runqueue.wait_on_pending_work())) {
        std::visit(work_visitor{this}, *task);
    }
}
} // namespace async::internal
