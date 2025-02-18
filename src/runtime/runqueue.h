#pragma once

#include <pch.h>
#include <runtime/coroutine.h>
#include <runtime/defines.h>

#include <iostream>
namespace async::runtime {

class runqueue {
  public:
    runqueue();

    void push_pending(task_block &block);
    void push_pending(task_block &&block);

    task_block peek_pending();
    void pop_pending();
    task_block peek_pop_pending();

    void activate(const task_block &block) {
        auto id = _coro_id.get();

        coro_block c_block{id, [func = block.func, id, this]() -> coroutine {
                               /*std::cout << "STARTED CORO INSIDE\n";*/
                               func();
                               /*std::cout << "AFTER TASK FUNC\n";*/
                               clean_coro(id);
                               co_return;
                           }()};

        /*std::cout << "ACTIVATED\n";*/
        std::lock_guard lck(_coroM);
        /*std::cout << "PUSH\n";*/
        _coroutines.push_back(c_block);
        /*std::cout << "PUSHed\n";*/
    }

    void shutdown() {
        for (auto &block : _coroutines) {
            block.coro.destroy();
        }
        _coroutines.clear();
    }

  private:
    void clean_coro(cid_t id) {
        std::lock_guard lck(_coroM);
        auto i = std::ranges::find_if(
            _coroutines, [id](coro_block &coro) { return coro.id == id; });

        if (i != std::end(_coroutines)) {
            _coroutines.erase(i);
        }
    }

    std::mutex _qM;
    std::condition_variable _q_signal;
    std::deque<task_block> _active;

    // TODO: optimize
    std::mutex _coroM;
    std::deque<coro_block> _coroutines;

    id_gen<cid_t> _coro_id;
};
} // namespace async::runtime
