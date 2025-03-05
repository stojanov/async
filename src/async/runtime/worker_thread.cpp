#include "async/defines.h"
#include "async/runtime/runqueue.h"
#include "async/utils.h"
#include <async/runtime/runtime_core.h>
#include <async/runtime/worker_thread.h>

namespace async::runtime {
worker_thread::worker_thread(runtime_core &core)
    : _core(core), _running(std::make_shared<std::atomic_bool>(true)) {}

void worker_thread::signal_shutdown() { _running->store(false); }
// consider exit case
void worker_thread::work() {
    auto work_visitor = var_overload{
        [this](task_block &block) { _core._runqueue.activate(block); },
        [this](coro_handle handle) { handle.resume(); },
    };

    while (*_running) {
        if (auto task = _core._runqueue.wait_on_pending_work()) {
            std::visit(work_visitor, *task);
        }
    }
}
} // namespace async::runtime
