#pragma once

#include <async/internal/pch.h>
#include <async/internal/runtime/runtime.h>
#include <async/internal/select_base.h>
#include <async/internal/util/signal_connection.h>

namespace async::internal {

struct select_core {
    // awaitable
    bool await_ready() { return _resolved_value; }

    void await_suspend(coro_handle h) { _h = h; }

    sequential_match_select await_resume() { return {_resolved_value}; }

    template <typename... Args> select_core(Args &&...args) {
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
inline static internal::select_core select(Args &&...args) {
    return {std::forward<Args>(args)...};
};

} // namespace async
