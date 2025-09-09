#include "async/defines.h"
#include "async/runtime/runqueue.h"
#include "async/utils.h"
#include <async/runtime/runtime_core.h>
#include <async/runtime/worker_thread.h>

namespace async::internal {

struct work_visitor {
    inline void operator()(task_package &task) {
        th->_core._runqueue.activate(task);
    }

    inline void operator()(coro_handle handle) { handle.resume(); }

    worker_thread *th;
};

// Refactor
worker_thread::worker_thread(runtime_core &core) : _core(core) {}

// consider exit case
void worker_thread::activate_pending_work() {
    if (auto task = _core._runqueue.wait_on_pending_work()) {
        std::visit(work_visitor{this}, *task);
    }
}
} // namespace async::internal
