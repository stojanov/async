#include "async/runtime/io_context_thread.h"
#include <async/runtime/runtime_core.h>
#include <async/runtime/timer_thread.h>
#include <async/runtime/worker_thread.h>
#include <coroutine>

namespace async::runtime {
runtime_core::runtime_core() : _visitor(*this) {}

void runtime_core::spawn(std::size_t N) {
    assert(N >= 2);

    const auto id = _id.get();
    worker_thread default_worker_thread = worker_thread(*this);

    for (int i = 0; i < N; i++) {
        auto thread_work_variant =
            std::make_unique<thread_var_t>(default_worker_thread);

        const auto thread_work = [this, work = thread_work_variant.get()]() {
            worker(work);
        };
        _threads.emplace_back(id, std::move(thread_work_variant),
                              std::make_unique<std::thread>(thread_work));
    }

    _running = true;
    _start_signal.notify_all();
    _capacity.fetch_add(N);
}

void runtime_core::submit(task_func &&func) {
    _runqueue.push_pending_raw_task({func});
}

// should be able to specify somekind of priority
void runtime_core::submit_resume(std::coroutine_handle<> h) {
    _runqueue.push_pending_resume(h);
};

void runtime_core::worker(thread_var_t *work) {
    {
        std::unique_lock lck(_start_M);
        _start_signal.wait(lck, [this]() { return _running.load(); });
    }
    while (_running) {
        std::visit(_visitor, *work);
    }
}

void runtime_core::shutdown() {
    _running = false;

    _runqueue.shutdown();

    const auto visitor = var_overload{
        [](worker_thread &thread) { thread.signal_shutdown(); },
        [](timer_thread &thread) { thread.cleanup(); },
        [](io_context_thread &thread) { thread.shutdown(); },
    };

    std::ranges::for_each(_threads, [&visitor](thread_block &block) {
        std::visit(visitor, *block.thread_work);
        // TODO: think about this, some that will actually release won't have
        // data inside needs to be covered by all the util/concurrent blocks
        /*block.active.resume();*/
        block.thread->join();
    });

    _threads.clear();
}
} // namespace async::runtime
