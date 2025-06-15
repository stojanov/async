#pragma once

#include <async/defines.h>
#include <algorithm>
#include <async/pch.h>
#include <async/runtime/runtime.h>
#include <optional>
#include <random>

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
        value_state_func func;
        cid_t id;
    };

  public:
    channel_core(cid_t id)
        : _rnd_gen(_rnd_dev()), _rnd_dist(0.f, 1.f), _id(id) {}

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

        const auto observers_value_pass_notification = [&]() {
            // Needs to be locked
            for (auto i = _observables.begin(); i != _observables.end(); i++) {
                if (i->func(value_state::NOTIFY, _id, std::any(value))) {
                    return true;
                }
            }
            return false;
        };

        if (!_observables.empty() && is_waiting_empty) {
            if (!observers_value_pass_notification()) {
                std::lock_guard lck(_queue_m);
                _queue.push(value);
                notify_observers(value_state::READY);
            }
            return;
        }

        if (!_observables.empty() && !is_waiting_empty) {
            if (_rnd_dist(_rnd_gen) > 0.5) {
                if (!observers_value_pass_notification()) {
                    goto consume_value_waiting;
                }
            } else {
            consume_value_waiting:
                waiting_values first;
                {
                    std::lock_guard lck(_waiting_m);
                    first = _waiting.front();
                    _waiting.pop_front();
                }

                first.awaitable->_value = value;

                runtime::runtime::get().submit_resume(first.handle);
            }
        }
    }

    chan_awaitable fetch() { return {*this}; }

    std::optional<T> try_fetch() {
        std::lock_guard lck(_queue_m);

        if (_queue.empty()) {
            return std::nullopt;
        }

        auto value = _queue.back();
        _queue.pop();

        // Todo: can this be optimized
        if (_queue.empty()) {
            notify_observers(value_state::CONSUMED);
        }

        return value;
    }

    void add_waiting(std::coroutine_handle<> h, chan_awaitable *awaitable) {
        std::lock_guard lck(_waiting_m);
        _waiting.emplace_back(h, awaitable);
    }

    void add_obeservable(cid_t observer_id, value_state_func func) {
        // lock
        if (!_queue.empty()) {
            func(value_state::READY, _id, std::nullopt);
        }

        _observables.emplace_back(func);
    }

    void remove_observable(cid_t id) {}

    void clean_up_waiting(std::coroutine_handle<> h) {
        std::lock_guard lck(_waiting_m);

        std::erase_if(_waiting, [h](waiting_values &waitier) {
            return waitier.handle == h;
        });
    }

    void notify_observers(value_state state,
                          std::optional<std::any> value = std::nullopt) {
        std::ranges::for_each(
            _observables, [&](auto &block) { block.func(state, _id, value); });
    }

    cid_t id() { return _id; }

  private:
    cid_t _id;

    std::mutex _queue_m;
    std::queue<T> _queue;

    std::mutex _waiting_m;
    std::deque<waiting_values> _waiting;

    std::mutex _observable_m;
    std::deque<observer_block> _observables;

    std::random_device _rnd_dev;
    std::mt19937 _rnd_gen;
    std::uniform_real_distribution<> _rnd_dist;
};

struct channel_base {
    virtual std::optional<std::any> try_fetch() = 0;

    virtual cid_t id() = 0;

    virtual void add_obeservable(cid_t id, value_state_func &&func) = 0;
    virtual void remove_observable(cid_t id) {};

    static inline std::atomic<cid_t> _s_id_counter{1};
};

template <typename T> struct channel : public channel_base {
    using value_type = T;

    channel() : _core(_s_id_counter.fetch_add(1)) {}
    ~channel() {
        // TODO: unblock all, maybe?
    }

    void push(const T &value) { _core.push(value); }

    // fetch await
    channel_core<T>::chan_awaitable fetch() { return _core.fetch(); }

    std::optional<std::any> try_fetch() final {
        auto v = _core.try_fetch();

        spdlog::warn("CORE FETCH CHANNEL");
        if (v == std::nullopt) {
            return std::nullopt;
        }

        return v.value();
    }

    cid_t id() { return _core.id(); }

  private:
    std::optional<std::any> try_fetch_non_notify(cid_t non_notify_id) {}

    void add_obeservable(cid_t id, value_state_func &&func) final {
        _core.add_obeservable(id, func);
    }

  private:
    channel_core<T> _core;
};

} // namespace async
