#pragma once

#include "defines.h"
#include <pch.h>
#include <runtime/coroutine.h>
#include <runtime/runtime_core.h>
#include <runtime/timer_thread_handler.h>

namespace async::runtime {

class runtime {
  public:
    runtime();
    static runtime &get();

    template <typename T> struct Closure {
        T state;
    };

    void submit(task_func &&task, int prio = 1);

    template <typename T>
    void submit_t(Closure<T> closure, closure_task_func<T> &&task,
                  int prio = 1);

    void shutdown();

    cid_t attach_timer(duration_t duration, bool rolling,
                       void_func on_timeout) {
        return _timer_th_handler.attach_timer(duration, rolling, on_timeout);
    }

    void remove_timer(cid_t id) { _timer_th_handler.remove_timer(id); }

  private:
    // priority, maybe ?
    std::queue<std::coroutine_handle<>> _to_resume;

    runtime_core _core;
    timer_thread_handler _timer_th_handler;
};

static inline auto &get() { return runtime::get(); }

} // namespace async::runtime
