#include <async/runtime/defines.h>
#include <async/runtime/runqueue.h>
#include <coroutine>

namespace async::runtime {
runqueue::runqueue() {}

void runqueue::push_pending_raw_task(task_block &block) {
    {
        std::unique_lock lck(_raw_pending_M);
        _pending_raw_tasks.push_back(block);
    }

    spdlog::warn("Got task");
    _raw_pending_signal.notify_one();
    _wait_task_signal.notify_one();
}

void runqueue::push_pending_raw_task(task_block &&block) {
    {
        std::unique_lock lck(_raw_pending_M);
        _pending_raw_tasks.push_back(std::move(block));
    }

    spdlog::warn("Got task");
    _raw_pending_signal.notify_one();
    _wait_task_signal.notify_one();
}

void runqueue::push_pending_resume(std::coroutine_handle<> h) {
    // should be locked
    {
        std::lock_guard lck(_pending_coro_res_M);
        _pending_coro_resumes.push_back(h);
    }
    // spdlog::warn("Got resume");
    _wait_task_signal.notify_one();
}

// noone uses this as of now
coro_handle runqueue::peek_pop_pending_resume() {
    // TODO: locking
    coro_handle h = _pending_coro_resumes.front();

    _pending_coro_resumes.pop_back();

    return h;
}

// TODO: CONSIDER EXIT CASE, SHUTDOWN
task_block runqueue::peek_pending_task() {
    std::unique_lock lck(_raw_pending_M);
    _raw_pending_signal.wait(lck,
                             [this]() { return !_pending_raw_tasks.empty(); });

    const task_block block = _pending_raw_tasks.front();

    return block;
}

std::optional<runqueue::task_object> runqueue::wait_on_pending_work() {
    {
        std::unique_lock lck(_wait_task_M);

        _wait_task_signal.wait(lck, [this]() {
            return !_pending_coro_resumes.empty() ||
                   !_pending_raw_tasks.empty();
        });
    }

    // TODO: merge theese into one, single variant can get the job done
    {
        std::lock_guard lck(_pending_coro_res_M);
        if (!_pending_coro_resumes.empty()) {
            coro_handle h = _pending_coro_resumes.front();
            // spdlog::warn("FOUND CORO");
            _pending_coro_resumes.pop_front();
            return h;
        }
    }

    {
        std::lock_guard lck(_raw_pending_M);
        if (!_pending_raw_tasks.empty()) {
            // minimize copy
            task_block t = _pending_raw_tasks.front();
            spdlog::warn("FOUND TASK");
            _pending_raw_tasks.pop_front();
            return t;
        }
    }

    return std::nullopt;
}

// TODO: add try_pop, when needed
void runqueue::pop_pending_taks() {
    std::unique_lock lck(_raw_pending_M);
    _raw_pending_signal.wait(lck,
                             [this]() { return !_pending_raw_tasks.empty(); });

    _pending_raw_tasks.pop_front();
}

task_block runqueue::peek_pop_pending_task() {
    std::unique_lock lck(_raw_pending_M);
    _raw_pending_signal.wait(lck,
                             [this]() { return !_pending_raw_tasks.empty(); });

    const task_block block = _pending_raw_tasks.front();
    _pending_raw_tasks.pop_front();

    return block;
}

} // namespace async::runtime
