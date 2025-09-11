#pragma once

#include <async/defines.h>
#include <async/pch.h>

#include <async/util/signal_connection.h>
#include <tuple>

namespace async::internal::utils {

template <typename T, bool Sync = true> class buffered_observer_signal {
    using func = std::function<T>;

    struct func_handle {
        u64 id{0};
        clk_t::time_point timestamp;
        func f;
    };

    const cid_t MAGIC = 0xDEADBEEF;

  public:
    buffered_observer_signal(std::size_t buffer_size)
        : _buffer_size(buffer_size) {}

    signal_connection connect(func &&callback) {
        const auto id = _id_counter.fetch_add(1);

        do_sync_op([&] { _funcs.emplace_back(id, clk_t::now(), callback); });

        const auto close_f = [this, id]() {
            // this is done, becase we are not ensuring the lifetime
            // caller/user is responsible for liffetime of observer_signal
            if (MAGIC != 0xDEADBEEF) {
                return;
            }
            close_connection(id);
        };

        apply_deffered(callback);

        return {std::move(close_f)};
    }

    void operator()() {
        for (auto const &handle : _funcs) {
            handle.f();
        }

        register_deffered_call();
    }

    template <typename... Args> void operator()(Args &...args) {
        do_sync_op([&] {
            // spdlog::warn("SIGNAL CALLED {}", _funcs.size());
            for (auto const &handle : _funcs) {
                handle.f(std::forward<Args>(args)...);
            }
        });

        register_deffered_call(args...);
    }

    template <typename... Args> void operator()(Args &&...args) {
        do_sync_op([&] {
            // spdlog::warn("SIGNAL CALLED {}", _funcs.size());
            for (auto const &handle : _funcs) {
                handle.f(std::forward<Args>(args)...);
            }
        });

        register_deffered_call(args...);
    }

  private:
    void apply_deffered(func &f) {
        for (auto &deffered : _deffered_calls) {
            deffered(f);
        }
    }

    void register_deffered_call() {
        auto callback = [](func &f) { f(); };

        _deffered_calls.emplace_back(std::move(callback));

        if (_deffered_calls.size() > _buffer_size) {
            _deffered_calls.erase(_deffered_calls.begin());
        }
    };

    template <typename... Args> void register_deffered_call(Args &&...args) {
        auto stored_args = std::make_tuple(std::forward<Args>(args)...);
        auto callback = [stored_args](func &f) { std::apply(f, stored_args); };

        _deffered_calls.emplace_back(std::move(callback));

        if (_deffered_calls.size() > _buffer_size) {
            _deffered_calls.erase(_deffered_calls.begin());
        }
    };

    inline void do_sync_op(std::function<void()> &&operation) {
        if constexpr (Sync) {
            std::lock_guard lck(_M);
            operation();
        } else {
            operation();
        }
    }

    void close_connection(u64 id) {
        // can be extended depending on the use case
        // in small sets this is faster than a map, in bigger ones
        // this could be swapped out with a better container
        // good enough for now
        // TODO: optimize this!
        spdlog::warn("CLOSIGN CONNECTION from inside SIGNAL {}", id);
        do_sync_op([&] {
            std::erase_if(_funcs, [id](const func_handle &handle) {
                return handle.id == id;
            });
        });
    }

  private:
    std::size_t _buffer_size;
    std::vector<func_handle> _funcs;
    std::deque<std::function<void(func &)>> _deffered_calls;
    std::atomic<cid_t> _id_counter{0};
    std::mutex _M;
};
} // namespace async::internal::utils
