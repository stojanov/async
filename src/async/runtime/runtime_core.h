#pragma once

#include <async/pch.h>
#include <async/runtime/coroutine.h>

#include <async/runtime/io_context_thread.h>
#include <async/runtime/runqueue.h>
#include <async/runtime/timer_thread.h>
#include <async/runtime/worker_thread.h>
#include <coroutine>

namespace async::internal {
struct runtime_core {
    using thread_var_t =
        std::variant<timer_thread, worker_thread, io_context_thread>;

    // struct thread_visitor;

    struct thread_block {
        cid_t id;
        u_ptr<thread_var_t> thread_work;
        u_ptr<std::thread> thread;
        // this isn't used
        std::coroutine_handle<> active;
    };

    struct thread_work_visitor {
        thread_work_visitor(runtime_core &core) : _core(core) {}

        void operator()(timer_thread &thread) {
            while (_core._running) {
                thread.run_timers();
            }
        }

        void operator()(worker_thread &thread) {
            while (_core._running) {
                _core._load.fetch_add(1);
                thread.activate_pending_work();
                _core._load.fetch_sub(1);
            }
        };

        void operator()(io_context_thread &thread) {
            while (_core._running) {
                thread.work();
            };
        }

      private:
        runtime_core &_core;
    };

    runtime_core();

    ~runtime_core() {
        std::cout << "RUNTIME DEASD\n";
        // _runqueue.print_stats();
    }

    void spawn(std::size_t N);

    template <is_in_variant_v<thread_var_t> T, typename... Args>
    const thread_block &spawn_new(Args &&...args) {
        const auto id = _id.get();

        auto thread_work_variant = std::make_unique<thread_var_t>();
        thread_work_variant->emplace<T>(std::forward<Args>(args)...);

        const auto thread_work = [this, work = thread_work_variant.get()]() {
            worker(work);
        };

        // TODO: LOCK HERE
        auto &t =
            _threads.emplace_back(id, std::move(thread_work_variant),
                                  std::make_unique<std::thread>(thread_work));

        _capacity.fetch_add(1);
        return t;
    }

    bool has_available();

    // provide raw, and coroutine spawns
    void submit(coroutine_void_func &&func);

    // pure func/task,
    // TODO: in the future maybe use the std::future abstraction
    // For the coroutines provide our own future base
    void submit(void_func &&func);

    template <typename T> void submit_closure(T &state, any_func func) {
        _runqueue.push_pending_raw_task({func, state});
    }

    void submit_resume(std::coroutine_handle<> h);

    void remove_coro(cid_t id) {
        spdlog::warn("REMOVE CORO");
        _runqueue.clean_coro(id);
    }

    void spawn_new();
    void shutdown();
    void worker(thread_var_t *work);

    std::mutex _start_M;
    std::condition_variable _start_signal;
    std::atomic_bool _running{false};
    std::atomic<std::size_t> _capacity;
    std::atomic<std::size_t> _load;

    std::list<thread_block> _threads;
    thread_work_visitor _thread_work_visitor;
    id_gen<cid_t> _id;

    // TODO: think about each thread to have it's own queue
    runqueue _runqueue;
};
} // namespace async::internal
