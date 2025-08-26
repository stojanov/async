#pragma once

#include <async/io/pal/io_handle.h>
#include <async/io/pal/io_op.h>
#include <async/pch.h>
#include <async/runtime/defines.h>
#include <async/runtime/io_thread_handler.h>
#include <async/runtime/runtime_core.h>
#include <async/runtime/timer_thread_handler.h>

namespace async::runtime {

class runtime {
  public:
    runtime();
    static runtime &get();

    // Submit a coroutine
    void submit_coro(coroutine_void_func &&task, int prio = 1);

    // Submit a pure task into the thread pool
    void submit_func(void_func &&task, int prio = 1);

    void submit_resume(coro_handle h);

    bool submit_io_op(s_ptr<io::pal::io_op> op);

    template <typename T>
    void submit_closure(T &state, closure_task_func<T> &&closure_func,
                        int prio = 1) {
        const auto task = [closure_func](std::any &state) {
            closure_func(std::any_cast<T &>(state));
        };

        submit_coro(task, prio);
    }

    void shutdown();

    cid_t attach_timer(duration_t duration, bool rolling,
                       void_func on_timeout) {
        return _timer_th_handler.attach_timer(duration, rolling, on_timeout);
    }

    void remove_timer(cid_t id) { _timer_th_handler.remove_timer(id); }

    void remove_coro();

  private:
    // TODO:
    // priority, maybe ?
    std::queue<std::coroutine_handle<>> _to_resume;

    runtime_core _core;
    timer_thread_handler _timer_th_handler;
    io_thread_handler _io_th_handler;
};

static inline auto &get() { return runtime::get(); }

static inline void submit(coroutine_void_func &&task, int prio = 1) {
    get().submit_coro(std::move(task), prio);
}

} // namespace async::runtime
