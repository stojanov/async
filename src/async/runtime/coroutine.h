#pragma once

#include <coroutine>
#include <spdlog/spdlog.h>

namespace async {

struct promise;

struct coroutine : std::coroutine_handle<promise> {
    using promise_type = async::promise;
};

struct promise {
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept;
    void return_void() {}
    void unhandled_exception() {}

    std::size_t _id;
};

} // namespace async
