#pragma once

#include <coroutine>
#include <memory>
#include <optional>
#include <pch.h>
#include <runtime/runtime.h>

namespace async {

template <typename T> struct channel_core {
  public:
    struct chan_awaitable {
        chan_awaitable(channel_core<T> &core) : _core(core) {}

        bool await_ready() {
            if (_value = _core.try_fetch(); _value) {
                return true;
            } else {
                return false;
            }
        }

        void await_suspend(std::coroutine_handle<> h) {
            if (_value) {
                h.resume();
            } else {
                _h = h;
                _core.add_waiting(h, this);
            }
        }

        std::optional<T> await_resume() {
            // this can go on another thread
            _core.clean_up_waiting(_h);

            return _value;
        }

      private:
        std::coroutine_handle<> _h;
        friend channel_core<T>;
        channel_core<T> &_core;

        // in case of timed wait, this has to be optional
        std::optional<T> _value;
    };

  private:
    struct waiting_values {
        std::coroutine_handle<> handle;
        chan_awaitable *awaitable;
    };

    struct observer_block {
        any_func func;
        cid_t id;
    };

  public:
    channel_core() {}

    // TODO: add move
    void push(const T &value) {
        bool is_waiting_empty = false;

        {
            std::lock_guard lck(_waiting_m);
            is_waiting_empty = _waiting.empty();
        }

        if (is_waiting_empty && _observables.empty()) {
            std::lock_guard lck(_queue_m);
            _queue.push(value);
            return;
        }

        if (!_observables.empty()) {
            auto observer = _observables.front();
            _observables.pop_back();
            // TODO:
            //
            auto k = std::any(value);

            /*std::cout << "FROM OBS " << k.type().name() << "\n";*/

            observer.func(std::any(value));
            return;
        }

        waiting_values first;
        {
            std::lock_guard lck(_waiting_m);
            first = _waiting.front();
            _waiting.pop_front();
        }

        first.awaitable->_value = value;

        runtime::runtime::get().submit([first]() { first.handle.resume(); }, 1);
    }

    chan_awaitable fetch() { return {*this}; }

    std::optional<T> try_fetch() {
        std::lock_guard lck(_queue_m);

        if (_queue.empty()) {
            return std::nullopt;
        }

        auto value = _queue.back();
        _queue.pop();

        return value;
    }

    void add_waiting(std::coroutine_handle<> h, chan_awaitable *awaitable) {
        std::lock_guard lck(_waiting_m);
        _waiting.emplace_back(h, awaitable);
    }

    void add_obeservable(cid_t id, any_func func) {
        auto found =
            std::ranges::find_if(_observables, [id](auto &observer_block) {
                return observer_block.id != id;
            });

        if (found == std::end(_observables)) {
            _observables.emplace_back(func);
        }
    }

    void remove_observable(cid_t id) {}

    void clean_up_waiting(std::coroutine_handle<> h) {
        std::lock_guard lck(_waiting_m);

        std::erase_if(_waiting, [h](waiting_values &waitier) {
            return waitier.handle == h;
        });
    }

  private:
    std::mutex _queue_m;
    std::queue<T> _queue;

    std::mutex _waiting_m;
    std::deque<waiting_values> _waiting;

    std::mutex _observable_m;
    std::deque<observer_block> _observables;
};

struct channel_base {
    virtual std::optional<std::any> try_fetch() = 0;
    virtual void add_obeservable(cid_t id, any_func &&func) = 0;
};

template <typename T> struct channel : public channel_base {
    using value_type = T;

    channel() {}
    ~channel() {
        // TODO: unblock all, maybe?
    }

    void push(const T &value) { _core.push(value); }

    channel_core<T>::chan_awaitable fetch() { return _core.fetch(); }

    std::optional<std::any> try_fetch() final {
        auto v = _core.try_fetch();

        if (v == std::nullopt) {
            return std::nullopt;
        }

        return v.value();
    }

    void add_obeservable(cid_t id, any_func &&func) final {
        _core.add_obeservable(id, func);
    }

  private:
    channel_core<T> _core;
};

} // namespace async
