#pragma once

#include <coroutine>
#include <functional>
#include <spdlog/spdlog.h>

namespace async::runtime {

struct promise;

struct coroutine : std::coroutine_handle<promise> {
    using promise_type = runtime::promise;
};

struct promise {
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept {
        spdlog::warn("FINAL SUSPEND CORO {}", _id);
        return {};
    }
    void return_void() {}
    void unhandled_exception() {}

    std::size_t _id;
};

} // namespace async::runtime
