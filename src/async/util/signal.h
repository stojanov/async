#pragma once

#include <async/defines.h>
#include <async/pch.h>

#include <async/util/signal_connection.h>

namespace async::internal::utils {

template <typename T, bool Sync = true> class observer_signal {
    using func = std::function<T>;

    struct func_handle {
        u64 id{0};
        func f;
    };

    const cid_t MAGIC = 0xDEADBEEF;

  public:
    ~observer_signal() {}

    signal_connection connect(func &&callback) {
        const auto id = _id_counter.fetch_add(1);

        spdlog::warn("---===CONNECT called");
        do_sync_op([&] {
            spdlog::warn("ADDING CALLBACk");
            _funcs.emplace_back(id, callback);
        });

        const auto close_f = [this, id]() {
            // this is done, becase we are not ensuring the lifetime
            // caller/user is responsible for liffetime of observer_signal
            if (MAGIC != 0xDEADBEEF) {
                return;
            }
            spdlog::warn(
                "------------------------------CLOSING singal conneciton");
            close_connection(id);
        };

        return {std::move(close_f)};
    }

    void operator()() {
        for (auto const &handle : _funcs) {
            handle.f();
        }
    }

    template <typename... Args> void operator()(Args &...args) {
        do_sync_op([&] {
            // spdlog::warn("SIGNAL CALLED {}", _funcs.size());
            for (auto const &handle : _funcs) {
                handle.f(std::forward<Args>(args)...);
            }
        });
    }

    template <typename... Args> void operator()(Args &&...args) {
        do_sync_op([&] {
            // spdlog::warn("SIGNAL CALLED {}", _funcs.size());
            for (auto const &handle : _funcs) {
                handle.f(std::forward<Args>(args)...);
            }
        });
    }

  private:
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
    std::vector<func_handle> _funcs;
    std::atomic<cid_t> _id_counter{0};
    std::mutex _M;
};
} // namespace async::internal::utils
