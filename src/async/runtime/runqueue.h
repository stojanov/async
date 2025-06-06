#pragma once

#include "async/defines.h"
#include <async/pch.h>
#include <async/runtime/coroutine.h>
#include <async/runtime/defines.h>

#include <condition_variable>
#include <coroutine>
#include <spdlog/spdlog.h>

#include <iostream>
#include <variant>
namespace async::runtime {

class runqueue {
  public:
    using task_object = std::variant<task_block, coro_handle>;

    runqueue();

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
        auto coro =
            coro_block{id, [&block, this](cid_t id) -> coroutine {
                           auto block_copy = block;

                           if (std::holds_alternative<any_func>(block.func)) {
                               auto func = std::get<any_func>(block.func);
                               func(block_copy.state);
                           } else {
                               auto func = std::get<task_func>(block.func);
                               func();
                           }
                           spdlog::error("CORO FINISHED FROM INSIDE");
                           /*self->clean_coro(id);*/
                           co_return;
                       }(id)};

        spdlog::warn("STARTED/ACTIVATED CORO {}", id);

        std::lock_guard lck(_coroM);
        _coroutines.push_back(std::move(coro));
    }

    void release() {
        for (auto &block : _coroutines) {
            block.coro.resume();
        }
    }

    void shutdown() {
        _is_dropped = true;

        for (auto i = _coroutines.begin(); i != _coroutines.end();) {
            std::cout << "====\t ==== DESTROYING " << i->id << "\n";
            i->coro.destroy();
            i = _coroutines.erase(i);
        }
    }

  private:
    void clean_coro(cid_t id) {
        std::lock_guard lck(_coroM);
        spdlog::warn("Cleaning up coro id: {}, coro size: {}", id,
                     _coroutines.size());
        auto i = std::find_if(_coroutines.begin(), _coroutines.end(),
                              [id](coro_block &coro) {
                                  std::cout << "ID OF CORO " << id << " MATCH "
                                            << coro.id << "\n";
                                  return coro.id == id;
                              });

        std::cout << i->id << "\n";
        i->coro.destroy();
        _coroutines.erase(i);
        std::cout << "CLEARING CORO " << id << "\n";
    }

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
    std::deque<coro_block> _coroutines;

    id_gen<cid_t> _coro_id;
    std::atomic_bool _is_dropped{false};
};
} // namespace async::runtime
