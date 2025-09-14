#pragma once

#include <async/internal/pch.h>
#include <async/internal/runtime/runtime.h>

namespace async {

namespace internal {

// TODO: Refactor this code
template <typename T> struct channel_core {
  public:
    struct chan_awaitable {
        chan_awaitable(channel_core<T> &core) : _core(core) {}

        bool await_ready() {
            if (_value = std::move(_core.try_fetch()); _value) {
                return true;
            } else {
                return false;
            }
        }

        void await_suspend(std::coroutine_handle<> h) {
            _h = h;
            _core.add_waiting(h, this);
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

    void print_stuff() {}

    // TODO: add move
    void push(const T &value) {
        bool is_waiting_empty = false;

        auto is_waiting_empty_f = [&]() {
            std::lock_guard lck(_waiting_m);
            is_waiting_empty = _waiting.empty();
            return _waiting.empty();
        };

        if (is_waiting_empty_f() && _observables.empty()) {
            std::lock_guard lck(_queue_m);
            _queue.push(value);
            return;
        }

        const auto observers_value_pass_notification = [&]() {
            // Needs to be locked
            for (auto i = _observables.begin(); i != _observables.end(); i++) {
                if (i->func(value_state::NOTIFY, _id, &value)) {
                    return true;
                }
            }
            return false;
        };

        if (!_observables.empty() && is_waiting_empty_f()) {
            if (!observers_value_pass_notification()) {
                {
                    std::lock_guard lck(_queue_m);
                    _queue.push(std::move(value));
                }

                notify_observers(value_state::READY);
            }
        } else if (!_observables.empty() && !is_waiting_empty_f()) {
            if (_rnd_dist(_rnd_gen) > 0.5) {
                if (!observers_value_pass_notification()) {
                    goto consume_value_waiting;
                }
            } else {
                goto consume_value_waiting;
            }
        } else if (!is_waiting_empty_f()) {
        consume_value_waiting:
            waiting_values first;

            {
                std::lock_guard lck(_waiting_m);
                first = _waiting.front();
                _waiting.pop_front();
            }

            bool set = false;

            // This feels hacky, not *proper*
            // in a perfect world and in a perfect code in theory this shouldn't
            // be needed but my code is far from perfect
            {
                std::lock_guard lck(_queue_m);

                if (!_queue.empty()) {
                    T temp = std::move(_queue.front());
                    _queue.pop();
                    _queue.push(std::move(value));
                    first.awaitable->_value = std::move(temp);
                    set = true;
                }
            }

            if (!set) {
                first.awaitable->_value = std::move(value);
            }

            internal::runtime::inst().submit_resume(first.handle);
        }
    }

    chan_awaitable fetch() { return {*this}; }

    std::optional<T> try_fetch() {
        std::lock_guard lck(_queue_m);

        if (_queue.empty()) {
            return std::nullopt;
        }

        auto value = _queue.front();

        _queue.pop();

        return value;
    }

    std::pair<std::optional<T>, bool> try_fetch_select() {
        std::lock_guard lck(_queue_m);

        bool is_consumed = false;

        if (_queue.empty()) {
            return {std::nullopt, is_consumed};
        }

        auto value = _queue.front();

        _queue.pop();

        // Todo: can this be optimized
        if (_queue.empty()) {
            is_consumed = true;
        }

        return {value, is_consumed};
    }

    void add_waiting(std::coroutine_handle<> h, chan_awaitable *awaitable) {
        std::lock_guard lck(_waiting_m);
        _waiting.emplace_back(h, awaitable);
    }

    value_state add_obeservable(cid_t observer_id, value_state_func func) {
        // think about locking here
        _observables.emplace_back(func);

        {
            std::lock_guard lck(_queue_m);
            return _queue.empty() ? value_state::CONSUMED : value_state::READY;
        }
    }

    void remove_observable(cid_t id) {}

    void clean_up_waiting(std::coroutine_handle<> h) {
        std::lock_guard lck(_waiting_m);

        std::erase_if(_waiting, [h](waiting_values &waitier) {
            return waitier.handle == h;
        });
    }

    void notify_observers(value_state state, void *o = nullptr) {
        std::ranges::for_each(_observables,
                              [&](auto &block) { block.func(state, _id, o); });
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
  protected:
    template <typename... Args> friend struct channel_select_core;

    virtual std::pair<std::optional<std::any>, bool> try_fetch_select() = 0;

    // this will be the pointer
    virtual cid_t id() = 0;

    // if true represents this channel has available value to take
    virtual value_state add_obeservable(cid_t id, value_state_func &&func) = 0;
    virtual void remove_observable(cid_t id) {};

    static inline std::atomic<cid_t> _s_id_counter{1};
};

} // namespace internal

template <typename T> struct channel : public internal::channel_base {
    using value_type = T;

    channel() : _core(_s_id_counter.fetch_add(1)) {}
    ~channel() {}

    void push(const T &value) { _core.push(value); }

    // debug
    void print_stuff() { _core.print_stuff(); }
    // fetch await
    internal::channel_core<T>::chan_awaitable fetch() { return _core.fetch(); }

    std::optional<T> try_fetch() { return _core.try_fetch(); }

  private:
    cid_t id() final { return _core.id(); }

    std::pair<std::optional<std::any>, bool> try_fetch_select() final {
        auto [v, consumed] = _core.try_fetch_select();

        if (v == std::nullopt) {
            return {std::nullopt, consumed};
        }

        return {std::move(v.value()), consumed};
    }

    internal::value_state
    add_obeservable(cid_t id, internal::value_state_func &&func) final {
        return _core.add_obeservable(id, std::move(func));
    }

  private:
    internal::channel_core<T> _core;
};

} // namespace async
