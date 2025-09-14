#pragma once

#include <async/internal/pch.h>
#include <async/internal/runtime/runtime.h>
#include <async/internal/util/sync_op.h>

namespace async {

namespace internal {

struct mutex_core {
    bool is_free() {
        std::lock_guard lck(_M);
        return _awaiting.empty();
    }

    void add_waiting(coro_handle h) {
        std::lock_guard lck(_M);
        _awaiting.push_back(h);
    }

    void unlock() {
        std::lock_guard lck(_M);
        auto first = _awaiting.front();
        _awaiting.pop_front();
        internal::runtime::inst().submit_resume(first);
    }

    // TODO: optimize
    std::list<coro_handle> _awaiting;
    std::mutex _M;
};

struct mutex_awaitable_t {
    mutex_awaitable_t(mutex_core &core) : _core(core) {}

    bool await_ready() { return _core.is_free(); }

    void await_suspend(coro_handle h) { _core.add_waiting(h); }

    void await_resume() {}

  private:
    mutex_core &_core;
};

} // namespace internal

struct mutex {
    internal::mutex_awaitable_t lock() { return {_core}; }

    void unlock() { _core.unlock(); }

  private:
    internal::mutex_core _core;
};
} // namespace async
