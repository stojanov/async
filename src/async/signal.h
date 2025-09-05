#pragma once

#include <async/pch.h>
#include <async/runtime/runtime.h>

namespace async {

namespace internal {

struct signal_core {
  public:
    struct waiting_block {
        std::coroutine_handle<> handle;
        predicate_func predicate;
    };

    void add_waiting(const waiting_block &block) { _waiting.push_back(block); }

    void notify_all() {
        for (auto it = _waiting.begin(); it != _waiting.end();) {
            auto &block = *it;

            if (block.predicate()) {
                internal::runtime::inst().submit_resume(block.handle);
                it = _waiting.erase(it);
            } else {
                it++;
            }
        }
    }

    // TODO: optimize
    std::list<waiting_block> _waiting;
};

// signal
struct pred_awaitable {
    friend signal_core;

    pred_awaitable(signal_core &core, predicate_func &&predicate)
        : _core(core), _predicate(std::move(predicate)) {}

    // check if it's available if signal
    bool await_ready() { return _predicate(); }

    void await_suspend(std::coroutine_handle<> h) {
        _core.add_waiting({h, _predicate});
    }

    void await_resume() {}

    const predicate_func _predicate;
    signal_core &_core;
};
} // namespace internal

struct signal {
    signal() {}

    internal::pred_awaitable wait_if(predicate_func &&func) {
        return {_core, std::move(func)};
    };

    void notify_all() { _core.notify_all(); }

  private:
    internal::signal_core _core;
};
} // namespace async
