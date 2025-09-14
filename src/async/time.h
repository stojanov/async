#pragma once

#include <async/defines.h>
#include <async/new_select.h>
#include <async/runtime/runtime.h>
#include <async/util/signal.h>
#include <async/util/signal_connection.h>

namespace async {

struct timer : internal::select_notify_base {
  private:
    timer(duration_t timeout) : _timeout(timeout) {
        _timer_id = internal::runtime::inst().attach_timer(
            timeout, false, [this]() { on_finish(); });
    }

    friend timer create_timer(duration_t timeout);

  public:
    struct awaitable {
        bool await_ready() { return _timer.is_ready(); }
        void await_suspend(internal::coro_handle h) { _timer.add_waiting(h); }
        void await_resume() {}

        timer &_timer;
    };

    awaitable wait() { return {*this}; }

    void cancel() { internal::runtime::inst().remove_timer(_timer_id); }
    void restart() {
        cancel();
        _timer_id = internal::runtime::inst().attach_timer(
            _timeout, false, [this]() { on_finish(); });
    }
    void cancel_notify() {
        cancel();
        on_finish();
    }

  private:
    void on_finish() {
        _finished = true;
        _on_notify(this, internal::value_state::READY);

        std::lock_guard lck(_waitingM);
        for (auto &handle : _waiting) {
            internal::runtime::inst().submit_resume(handle);
        }
        _waiting.clear();
    };

    void add_waiting(internal::coro_handle handle) {
        std::lock_guard lck(_waitingM);
        _waiting.emplace_back(handle);
    }

  private:
    // ============== select_notify_base
    bool is_ready() { return _finished; }
    const std::type_info &type() { return typeid(int); };

    std::pair<internal::utils::signal_connection, internal::value_state>
    add_obeservable(internal::select_kind kind,
                    internal::select_notify_func &&func) final {
        auto state = is_ready() ? internal::value_state::READY
                                : internal::value_state::CONSUMED;
        switch (kind) {
        case internal::select_kind::ONE_SHOT: {
            internal::utils::signal_connection conn;
            if (state != internal::value_state::READY) {
                conn = _on_notify.connect(std::move(func));
            }
            return {conn, state};
        }
        case internal::select_kind::PERSISTENT: {
            auto conn = _on_notify.connect(std::move(func));
            return {conn, state};
        }
        }
    }

    internal::utils::observer_signal<bool(internal::select_notify_base *,
                                          internal::value_state)>
        _on_notify;

    duration_t _timeout;
    std::atomic_bool _finished{false};
    std::mutex _waitingM;
    std::list<internal::coro_handle> _waiting;
    std::atomic<cid_t> _timer_id;
};

inline timer create_timer(duration_t timeout) { return {timeout}; }

inline cid_t create_timer_callback(duration_t timeout, void_func &&callback) {
    return internal::runtime::inst().attach_timer(timeout, false, [callback]() {
        try {
            callback();
        } catch (...) {
        }
    });
}
inline void remove_timer_callback(cid_t id) {
    internal::runtime::inst().remove_timer(id);
}

} // namespace async
