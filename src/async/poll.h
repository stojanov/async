#pragma once

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

} // namespace detail

inline static auto poll(int prio = 0) { return detail::poll_awaitable{prio}; }

} // namespace async
