#pragma once

#include "async/defines.h"
#include <async/pch.h>
#include <async/runtime/runtime.h>

#include <spdlog/spdlog.h>

namespace async {
namespace detail {

struct poll_awaitable {
    poll_awaitable(int prio) : _prio(prio) {};

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        runtime::get().submit_resume(h);
    }

    bool await_resume() { return true; }

    int _prio;
};

struct timed_poll_awaitable {
    timed_poll_awaitable(duration_t timeout) : _dur{timeout} {}

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        runtime::get().attach_timer(
            _dur, false, [this, h]() { runtime::get().submit_resume(h); });
    }

    bool await_resume() { return true; }

    duration_t _dur;
};

} // namespace detail

inline static auto poll(int prio = 0) { return detail::poll_awaitable{prio}; }
inline static auto timed_poll(duration_t duration) {
    return detail::timed_poll_awaitable{duration};
}
} // namespace async
