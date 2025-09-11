#pragma once

#include <async/defines.h>
#include <async/pch.h>
#include <async/runtime/coroutine.h>
#include <async/runtime/defines.h>

#include <condition_variable>
#include <coroutine>
#include <spdlog/spdlog.h>
#include <variant>

namespace async::internal {

class runqueue {
  public:
    struct pending_task {
        cid_t id;
        task_package task;
    };
    using task_object = std::variant<pending_task, coro_handle>;

    runqueue();
    ~runqueue() {}

    void print_stats() {
        spdlog::warn("Runqueue destructor count items: resumes {}, tasks {}",
                     _pending_coro_resumes.size(), _pending_raw_tasks.size());
    }

    template <is_in_variant_v<task_package> T>
    cid_t push_pending_task(T &&task) {
        cid_t id = _coro_id.get();

        {
            std::lock_guard lck(_wait_task_M);

            _pending_raw_tasks.emplace_back(id, std::move(task));
        }

        _wait_task_signal.notify_one();
        return id;
    }

    void push_pending_resume(std::coroutine_handle<> h);

    // TODO: this also needs to be fast
    std::optional<task_object> wait_on_pending_work();

    void activate(const pending_task &task) {
        bool expected = true;
        if (_is_dropped.compare_exchange_strong(expected, true)) {
            return;
        }

        const auto visitor = var_overload{
            [](void_func func) { func(); },
            [&](const s_ptr<pending_coro_base> &pending_coro) {
                auto handle = pending_coro->construct(task.id);

                {
                    std::lock_guard lck(_coroM);
                    _coroutines.insert(std::pair{
                        task.id, coro_block{task.id, handle, pending_coro}});
                }

                // Start the coro since it's suspended at
                // construction
                // should here the _coroM mutex be locked, point to
                // think about
                spdlog::warn("STARTING CORO");
                handle.resume();
            }};

        std::visit(visitor, task.task);
    }

    void release() {
        // for (auto &block : _coroutines) {
        //     block.coro.resume();
        // }
    }

    void shutdown() {
        _is_dropped = true;

        std::lock_guard lck(_coroM);
        for (auto i = _coroutines.begin(); i != _coroutines.end();) {
            // most surely will fuck something up :D
            i->second.coro.destroy();
        }

        _coroutines.clear();
    }

    void clean_coro(cid_t id) {
        std::lock_guard lck(_coroM);

        if (auto i = _coroutines.find(id); i != std::end(_coroutines)) {
            _coroutines.erase(i);
        }
    }

  private:
    std::mutex _wait_task_M;
    std::condition_variable _wait_task_signal;

    std::mutex _raw_pending_M;
    std::condition_variable _raw_pending_signal;
    // TODO: Can this be optimized, shoud it be ?, lockless
    std::deque<pending_task> _pending_raw_tasks;

    std::mutex _pending_coro_res_M;
    std::condition_variable _pending_coro_res_signal;
    // TODO: Can this be optimized, shoud it be ?, lockless
    std::deque<coro_handle> _pending_coro_resumes;

    std::mutex _coroM;
    std::map<cid_t, coro_block> _coroutines;

    id_gen<cid_t> _coro_id;
    std::atomic_bool _is_dropped{false};
};
} // namespace async::internal
