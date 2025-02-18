#include <runtime/runtime_core.h>
#include <runtime/worker_thread.h>

namespace async::runtime {
worker_thread::worker_thread(runtime_core &core)
    : _core(core), _running(std::make_shared<std::atomic_bool>(true)) {}

void worker_thread::signal_shutdown() { _running->store(false); }
// consider exit case
void worker_thread::work() {
    while (*_running) {
        /*std::cout << "WORK STARTED\n";*/
        auto block = _core._runqueue.peek_pop_pending();

        /*std::cout << "GOT TASK STARTED\n";*/
        _core._runqueue.activate(block);
    }
}
} // namespace async::runtime
