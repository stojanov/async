#pragma once

#include <coroutine>

namespace async::runtime {

struct promise;

struct coroutine : std::coroutine_handle<promise> {
    using promise_type = runtime::promise;
};

struct promise {
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
};

} // namespace async::runtime
