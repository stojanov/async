#pragma once

#include <async/internal/runtime/runtime.h>
#include <async/internal/runtime/task_handle.h>

namespace async::runtime {
// Conf init struct
static inline void init(const init_config &config) {
    internal::runtime::init(config);
};

// Will crash if the runtime isn't created
static inline void shutdown() { internal::runtime::inst().shutdown(); }

} // namespace async::runtime

namespace async {

template <typename F>
    requires std::invocable<F> &&
             async::internal::is_specialization_of_v<std::invoke_result_t<F>,
                                                     coroutine>
static inline auto submit(F &&task, int prio = 1) {
    using ret_type = std::invoke_result_t<F>;
    using value_type = ret_type::value_type;
    return internal::runtime::public_inst().submit_coro<value_type>(
        std::move(task));
}

static inline void submit_func(void_func &&task, int prio = 1) {
    internal::runtime::public_inst().submit_func(std::move(task), 1);
}
} // namespace async
