#pragma once

#include <async/pch.h>
#include <async/runtime/runtime.h>

namespace async {

namespace internal {

struct waitgroup_core {
    waitgroup_core(std::size_t N) : _n(N) {}

    bool is_finished() {
        std::lock_guard lck(_countM);
        return _finished_count == _n;
    }

    void finish() {
        std::lock_guard lck(_countM);

        _finished_count += 1;

        if (_finished_count == _n) {
            _finished_count = 0;

            std::lock_guard lck(_waitingM);
            auto i = _waiting.begin();
            while (true) {
                internal::runtime::inst().submit_resume(*i);
                i = _waiting.erase(i);
            }
        }
    }

    void reset() {
        std::lock_guard lck(_countM);
        _finished_count = 0;
    }

    void attach_waiting(coro_handle h) {
        std::lock_guard lck(_waitingM);
        _waiting.push_back(h);
    }

  private:
    std::mutex _countM;
    std::size_t _finished_count{0};
    std::size_t _n;

    // TODO: lock waiting
    std::mutex _waitingM;
    std::list<coro_handle> _waiting;
};

struct waitgroup_awaitable_t {
    waitgroup_awaitable_t(waitgroup_core &core) : _core(core) {}

    bool await_ready() { return _core.is_finished(); }

    void await_suspend(coro_handle h) { _core.attach_waiting(h); }

    void await_resume() {}

  private:
    waitgroup_core &_core;
};
} // namespace internal

struct waitgroup {
    waitgroup(std::size_t N) : _core(N) {}

    void finish() { _core.finish(); }

    // Thread carefully with this method
    void reset() { _core.reset(); }

    internal::waitgroup_awaitable_t wait() { return {_core}; }

  private:
    internal::waitgroup_core _core;
};
} // namespace async
