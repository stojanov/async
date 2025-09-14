#include <async/internal/defines.h>
#include <async/internal/runtime/runtime.h>

namespace async::internal {

std::unique_ptr<runtime> runtime::s_runtime;

runtime::runtime(std::size_t thread_count)
    : _timer_th_handler(_core), _io_th_handler(_core) {
    _core.spawn(thread_count);
}

runtime::~runtime() { _core.shutdown(); }

runtime &runtime::inst() { return *s_runtime; }

void runtime::init(const async::runtime::init_config &config) {
    s_runtime = std::make_unique<runtime>(config.thread_count);
}

runtime &runtime::public_inst() {
    // this is done here, and not in the inst() method
    // since i want to avoid as much as possible branches or any other slowing
    // code that part might be called an obscene ammount of times
    // this comes with a rule that every helper/util/class such as
    // channel/poll/select etc etc.. cannot in the constructor call inst() only
    // when an awaitable is produced or in other words only an awaitable can use
    // the runtime since then we can guarantee it's created
    if (!s_runtime) {
        s_runtime = std::make_unique<runtime>(ASYNC_DEFAULT_THREAD_COUNT);
    }
    return *s_runtime;
}

void runtime::submit_func(void_func &&func, int prio) {
    _core.submit_func(std::move(func));
}

void runtime::submit_resume(coro_handle h) { _core.submit_resume(h); }

bool runtime::submit_io_op(s_ptr<io::pal::io_op> op) {
    return _io_th_handler.submit_io_op(op);
}

void runtime::shutdown() { s_runtime.reset(); }
} // namespace async::internal
