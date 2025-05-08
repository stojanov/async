#include <async/defines.h>
#include <async/runtime/runtime.h>

namespace async::runtime {
runtime::runtime() : _timer_th_handler(_core), _io_th_handler(_core) {
    _core.spawn(10);
}

runtime &runtime::get() {
    static runtime r;
    return r;
}

void runtime::submit(task_func &&func, int prio) {
    _core.submit(std::move(func));
}

void runtime::submit_resume(coro_handle h) { _core.submit_resume(h); }

bool runtime::submit_io_op(s_ptr<io::pal::io_op> op) {
    return _io_th_handler.submit_io_op(op);
}

void runtime::shutdown() { _core.shutdown(); }
} // namespace async::runtime
