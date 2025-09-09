#include <async/defines.h>
#include <async/runtime/runtime.h>

namespace async::internal {
runtime::runtime() : _timer_th_handler(_core), _io_th_handler(_core) {
    _core.spawn(4);
}

runtime &runtime::inst() {
    static runtime r;
    return r;
}

void runtime::submit_func(void_func &&func, int prio) {
    _core.submit_func(std::move(func));
}

void runtime::submit_resume(coro_handle h) {
    spdlog::warn("RESUME");
    _core.submit_resume(h);
}

bool runtime::submit_io_op(s_ptr<io::pal::io_op> op) {
    return _io_th_handler.submit_io_op(op);
}

void runtime::shutdown() { _core.shutdown(); }
} // namespace async::internal
