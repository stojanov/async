#pragma once

#include <async/pch.h>
#include <async/runtime/runtime.h>
#include <async/util/signal.h>
#include <async/util/signal_connection.h>
#include <concepts>

namespace async::internal {};

namespace async::internal {

enum class select_kind { ONE_SHOT, PERSISTENT };

struct select_notify_base;

using select_notify_func =
    std::function<bool(select_notify_base *, value_state)>;

struct select_notify_base {
  public:
    u64 id() { return (u64)this; }

    virtual bool is_ready() = 0;
    virtual const std::type_info &type() = 0;

    virtual std::pair<utils::signal_connection, value_state>
    add_obeservable(select_kind, select_notify_func &&func) = 0;
};

using select_handle = internal::select_notify_base *;

struct sequential_match_select {
    friend struct select_core_new;

  private:
    sequential_match_select(select_handle handle) : _handle(handle) {}

  public:
    template <typename T, typename F>
        requires std::invocable<F, T &>
    void on(T &o, F &&callback) {
        select_handle s = &o;

        if (_handle == s) {
            callback(o);
        }
    }

    template <typename T, typename F>
        requires std::invocable<F>
    void on(T &o, F &&callback) {
        select_handle s = &o;

        if (_handle == s) {
            callback();
        }
    }

    template <typename T, typename F>
        requires std::invocable<F, T &>
    void on(std::shared_ptr<T> &o, F &&callback) {
        on(*o, callback);
    }

    template <typename T, typename F>
        requires std::invocable<F>
    void on(std::shared_ptr<T> &o, F &&callback) {
        on(*o, callback);
    }

  private:
    select_handle _handle;
};

struct select_core_new {
    // awaitable
    bool await_ready() { return _resolved_value; }

    void await_suspend(coro_handle h) { _h = h; }

    sequential_match_select await_resume() { return {_resolved_value}; }

    template <typename... Args> select_core_new(Args &&...args) {
        (attach(std::forward<Args>(args)), ...);
    }

  private:
    template <typename T>
        requires std::is_base_of_v<internal::select_notify_base, T>
    void attach(T &notify_base) {
        if (_already_resolved) {
            return;
        }

        internal::select_notify_base *base = &notify_base;

        spdlog::error("Base ID {}, From ptr id {}", base->id(),
                      (std::uint64_t)base);

        auto [conn, state] = base->add_obeservable(
            select_kind::ONE_SHOT,
            [this](select_notify_base *handle, value_state) {
                return on_finish(handle);
            });

        spdlog::warn("== added observable");
        if (state == value_state::READY) {
            spdlog::warn("== STATE WEIRD ");
            _already_resolved = true;
            _resolved_value = base;
            return;
        };

        spdlog::warn("created scoped conneciton");
        _connections.emplace_back(std::move(conn));
    }

    template <typename T>
        requires std::is_base_of_v<internal::select_notify_base, T>
    void attach(std::shared_ptr<T> &notify_base) {
        attach(*notify_base);
    }

    bool on_finish(select_notify_base *base, value_state = value_state::NOOP) {
        spdlog::warn("============================================ NOTIFIED {}",
                     _connections.size());
        _resolved_value = base;
        runtime::inst().submit_resume(_h);
        return true;
    }

  private:
    select_notify_base *_resolved_value{nullptr};
    coro_handle _h;

  private:
    bool _already_resolved{false};
    std::list<internal::utils::scoped_signal_connection> _connections;
};

} // namespace async::internal

namespace async {

template <typename... Args>
inline static internal::select_core_new select_test(Args &&...args) {
    return {std::forward<Args>(args)...};
};

} // namespace async
