#pragma once

#include <async/io/pal/io_handle.h>
#include <async/io/pal/io_op.h>
#include <async/pch.h>
#include <async/runtime/defines.h>
#include <async/runtime/io_thread_handler.h>
#include <async/runtime/runtime_core.h>
#include <async/runtime/task_handle_base.h>
#include <async/runtime/timer_thread_handler.h>
#include <async/utils.h>

namespace async {
template <typename T> struct task_handle;
}

namespace async::internal {

class runtime {
  public:
    runtime();
    static runtime &inst();

    // Submit a pure task into the thread pool
    void submit_func(void_func &&task, int prio = 1);

    // legacy, test
    template <typename F>
        requires std::invocable<F> &&
                 is_specialization_of_v<std::invoke_result_t<F>, coroutine>
    void submit_test(F &&func) {
        using ret_type = std::invoke_result_t<F>;
        using value_type = ret_type::value_type;
        _core.submit_coro<value_type>(func);
    }

    template <typename T>
    s_ptr<task_handle<T>> submit_coro(std::function<coroutine<T>()> &&func) {
        auto id = _core.submit_coro<T>(std::move(func));
        auto handle = std::shared_ptr<task_handle<T>>(new task_handle<T>(id));

        {
            std::lock_guard lck(_task_handlesM);
            _task_handles.insert(std::pair{
                id, std::dynamic_pointer_cast<task_handle_base>(handle)});
        }

        return handle;
    }

    void submit_resume(coro_handle h);

    bool submit_io_op(s_ptr<io::pal::io_op> op);

    // Public: TODO: safe version of the attach_timer method
    cid_t submit_timer(duration_t duration, bool rolling, void_func on_timeout);

    // Private call
    cid_t attach_timer(duration_t duration, bool rolling,
                       void_func on_timeout) {
        return _timer_th_handler.attach_timer(duration, rolling, on_timeout);
    }

    void remove_timer(cid_t id) { _timer_th_handler.remove_timer(id); }

    void notify_result(cid_t id, void *o) {
        std::lock_guard lck(_task_handlesM);

        spdlog::warn("NOTIFY ID {}", id);
        if (auto i = _task_handles.find(id); i != std::end(_task_handles)) {
            i->second->on_result(o);
            _task_handles.erase(i);
        }
    }

    void shutdown();

  private:
    runtime_core _core;
    timer_thread_handler _timer_th_handler;
    io_thread_handler _io_th_handler;

    std::mutex _task_handlesM;
    std::map<cid_t, s_ptr<task_handle_base>> _task_handles;
};

static inline auto &get() { return runtime::inst(); }

} // namespace async::internal

namespace async::runtime {
// Conf init struct
static inline void init() {

};

template <typename F>
    requires std::invocable<F> &&
             async::internal::is_specialization_of_v<std::invoke_result_t<F>,
                                                     coroutine>
static inline auto submit(F &&task, int prio = 1) {
    using ret_type = std::invoke_result_t<F>;
    using value_type = ret_type::value_type;
    return internal::runtime::inst().submit_coro<value_type>(std::move(task));
}

static inline void submit_func(void_func &&task, int prio = 1) {
    internal::runtime::inst().submit_func(std::move(task), 1);
}

// template <typename T>
// static inline void submit_closure(T &state, closure_task_func<T> &&func,
//                                   int prio = 1) {
//     internal::runtime::inst().submit_closure<T>(state, func);
// }

} // namespace async::runtime
