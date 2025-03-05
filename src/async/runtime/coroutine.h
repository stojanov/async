#pragma once

#include <async/defines.h>
#include <coroutine>
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
        /*spdlog::info("CORO DIEDED {}", _id);*/
        return {};
    }
    void return_void() {}
    void unhandled_exception() {}

    /*cid_t _id;*/
    /*promise(cid_t id) : _id(id) {}*/
};

} // namespace async::runtime
