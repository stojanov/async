#pragma once

#include <pch.h>

namespace async {
namespace detail {

struct poll_awaitable {
    poll_awaitable(int prio) : _prio(prio) {};

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        /*runtime::get().submit_resume(h, _prio);*/
    }

    void await_resume() {}

    int _prio;
};

} // namespace detail

inline static auto poll(int prio) { return detail::poll_awaitable{prio}; }

} // namespace async
