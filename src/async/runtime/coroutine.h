#pragma once

#include <coroutine>
#include <functional>
#include <spdlog/spdlog.h>

namespace async::runtime {

struct promise;

struct coroutine : std::coroutine_handle<promise> {
    using promise_type = async::runtime::promise;

    std::size_t _id;
};

struct promise {
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept {
        // spdlog::info("CORO DIEDED {}");
        return {};
    }
    void return_void() { spdlog::info("CORO DIEDED {}"); }
    void unhandled_exception() {}

    /*cid_t _id;*/
    /*promise(cid_t id) : _id(id) {}*/
};

} // namespace async::runtime
