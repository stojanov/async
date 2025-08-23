#pragma once

#include <async/pch.h>
#include <async/runtime/coroutine.h>

namespace async::runtime {

struct task_block {
    std::variant<coroutine_void_func, coroutine_any_func> func;
    // think about unnecessary copying
    std::any state;
};

struct coro_block {
    // might not be needed to also store the id
    cid_t id;
    std::coroutine_handle<> coro;
    std::any state;
};

template <typename T> struct coro_block_closure {
    cid_t id;
    std::coroutine_handle<> coro;
    T closure;
};

} // namespace async::runtime
