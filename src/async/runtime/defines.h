#pragma once

#include <async/pch.h>
#include <async/runtime/coroutine.h>

namespace async::internal {

struct pending_coro_base {
    virtual ~pending_coro_base() = default;

    virtual coro_handle construct(cid_t id) = 0;
};

template <typename T> struct pending_coro : public pending_coro_base {
    pending_coro(std::function<coroutine<T>()> &&func)
        : work(std::move(func)) {}

    std::function<coroutine<T>()> work;

    coro_handle construct(cid_t id) override {
        coroutine<T> coro = work();
        coro.promise()._id = id;
        return coro;
    }
};

using task_package = std::variant<void_func, s_ptr<pending_coro_base>>;

// optimize this
struct coro_block {
    // might not be needed to also store the id
    cid_t id;
    std::coroutine_handle<> coro;

    // We need to hold this alive
    // basically means that we need to hold the "work" member aka
    // the function capture/closure alive until the end of the coro
    // so the captured data is alive
    s_ptr<pending_coro_base> base_coro;
};

} // namespace async::internal
