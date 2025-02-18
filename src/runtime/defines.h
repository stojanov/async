#pragma once

#include <pch.h>
#include <runtime/coroutine.h>

namespace async::runtime {

struct task_block {
    task_func func;
};

struct coro_block {
    cid_t id;
    std::coroutine_handle<> coro;
};

template <typename T> struct coro_block_closure {
    cid_t id;
    std::coroutine_handle<> coro;
    T closure;
};

} // namespace async::runtime
