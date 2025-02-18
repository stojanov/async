#include "runtime/runtime.h"
#include "defines.h"

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

void runtime::shutdown() { _core.shutdown(); }
} // namespace async::runtime
