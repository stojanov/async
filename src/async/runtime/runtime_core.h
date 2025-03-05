#pragma once

#include <async/pch.h>
#include <async/runtime/coroutine.h>

#include <async/runtime/runqueue.h>
#include <async/runtime/timer_thread.h>
#include <async/runtime/worker_thread.h>
#include <coroutine>

namespace async::runtime {
struct runtime_core {
    using thread_var_t = std::variant<timer_thread, worker_thread>;

    struct thread_visitor;

    struct thread_block {
        cid_t id;
        u_ptr<thread_var_t> thread_work;
        u_ptr<std::thread> thread;
        std::coroutine_handle<> active;
    };

    struct thread_visitor {
        thread_visitor(runtime_core &core) : _core(core) {}

        void operator()(timer_thread &thread) {
            work_fun([&](runtime_core &core) { thread.work(); });
        };

        void operator()(worker_thread &thread) {
            work_fun([&](runtime_core &core) { thread.work(); });
        };

        void on_start() { _core._load.fetch_add(1); }
        void on_finish() { _core._load.fetch_sub(1); }
        void work_fun(std::function<void(runtime_core &core)> f) {
            on_start();
            f(_core);
            on_finish();
        }

      private:
        runtime_core &_core;
    };

    runtime_core();
    ~runtime_core() { std::cout << "RUNTIME DEASD\n"; }

    void spawn(std::size_t N);

    template <typename T, typename... Args>
    const thread_block &spawn_new(Args &&...args) {
        const auto id = _id.get();

        auto thread_work_variant = std::make_unique<thread_var_t>();
        thread_work_variant->emplace<T>(std::forward<Args...>(args)...);

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
    void submit(task_func &&func);
    void submit_resume(std::coroutine_handle<> h);

    void spawn_new();
    void shutdown();
    void worker(thread_var_t *work);

    std::mutex _start_M;
    std::condition_variable _start_signal;
    std::atomic_bool _running{false};
    std::atomic<std::size_t> _capacity;
    std::atomic<std::size_t> _load;

    std::list<thread_block> _threads;
    thread_visitor _visitor;
    id_gen<cid_t> _id;

    // TODO: think about each thread to have it's own queue
    runqueue _runqueue;
};
} // namespace async::runtime
