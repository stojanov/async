#include <async/defines.h>
#include <async/runtime/runtime.h>

namespace async::runtime {
runtime::runtime() : _timer_th_handler(_core) { _core.spawn(10); }

runtime &runtime::get() {
    static runtime r;
    return r;
}

void runtime::submit(task_func &&func, int prio) {
    /*std::cout << "SUBMITTING\n";*/
    _core.submit(std::move(func));
}

void runtime::submit_resume(coro_handle h) {}

void runtime::shutdown() { _core.shutdown(); }
} // namespace async::runtime
