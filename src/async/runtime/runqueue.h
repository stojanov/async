#pragma once

#include <async/defines.h>
#include <async/pch.h>
#include <async/runtime/coroutine.h>
#include <async/runtime/defines.h>

#include <condition_variable>
#include <coroutine>
#include <spdlog/spdlog.h>

namespace async::internal {

class runqueue {
  public:
    using task_object = std::variant<task_block, coro_handle>;

    runqueue();

    void print_stats() {
        spdlog::warn("Runqueue destructor count items: resumes {}, tasks {}",
                     _pending_coro_resumes.size(), _pending_raw_tasks.size());
    }

    ~runqueue() {}

    void push_pending_raw_task(task_block &block);
    void push_pending_raw_task(task_block &&block);

    void push_pending_resume(std::coroutine_handle<> h);

    std::optional<task_object> wait_on_pending_work();

    task_block peek_pending_task();
    void pop_pending_taks();
    task_block peek_pop_pending_task();
    coro_handle peek_pop_pending_resume();

    void activate(const task_block &block) {
        bool expected = true;
        if (_is_dropped.compare_exchange_strong(expected, true)) {
            return;
        }

        auto id = _coro_id.get();
        decltype(_coroutines)::iterator entry;

        {
            std::lock_guard lck(_coroM);

            auto coro_entry = _coroutines.insert(
                std::pair{id, coro_block{id, nullptr, block.state}});

            if (!coro_entry.second) {
                // TODO: handle
                return;
            }

            entry = coro_entry.first;
        }

        coroutine coro;

        // TODO: possibility for this code to be moved elsewhere
        // runqueue gets too cluttered, as it storing delagating and activating
        // work/coroutines it gets confusing
        const auto visitor = var_overload{
            [&](coroutine_any_func func) { coro = func(entry->second.state); },
            [&](coroutine_void_func func) { coro = func(); },
            // This will block unlike the coroutines, it's actual pure work old
            // fashioned blocking work
            [](void_func func) {
                spdlog::warn("NORMAL FUNC!!!");
                func();
            },
        };

        std::visit(visitor, block.func);

        if (coro) {
            spdlog::warn("Added the coro inside the entry");

            std::coroutine_handle<promise> handle = coro;
            handle.promise()._id = id;

            entry->second.coro = coro;
            coro.resume();
        } else {
            _coroutines.erase(entry);
        }
    }

    void release() {
        // for (auto &block : _coroutines) {
        //     block.coro.resume();
        // }
    }

    void shutdown() {
        _is_dropped = true;

        for (auto i = _coroutines.begin(); i != _coroutines.end();) {
            // std::cout << "====\t ==== DESTROYING " << i->id << "\n";
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
    std::deque<task_block> _pending_raw_tasks;

    std::mutex _pending_coro_res_M;
    std::condition_variable _pending_coro_res_signal;
    // TODO: Can this be optimized, shoud it be ?, lockless
    std::deque<coro_handle> _pending_coro_resumes;

    // TODO: optimize, fenwick tree, binary indexed tree, segment tree
    std::mutex _coroM;
    std::map<cid_t, coro_block> _coroutines;
    // VERY STUPID, find a way for this to be better
    // std::map<coro_handle, cid_t> _corohandle_cid;

    id_gen<cid_t> _coro_id;
    std::atomic_bool _is_dropped{false};
};
} // namespace async::internal
