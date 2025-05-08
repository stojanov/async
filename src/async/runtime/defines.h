#pragma once

#include <async/pch.h>
#include <async/runtime/coroutine.h>

namespace async::runtime {

struct task_block {
    std::variant<task_func, any_func> func;
    // think about unnecessary copying
    std::any state;
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
