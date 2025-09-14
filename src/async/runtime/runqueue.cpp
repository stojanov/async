#include <async/runtime/defines.h>
#include <async/runtime/runqueue.h>
#include <coroutine>

namespace async::internal {
runqueue::runqueue() {}

void runqueue::push_pending_resume(std::coroutine_handle<> h) {
    // should be locked
    {
        std::lock_guard lck(_wait_task_M);
        _pending_coro_resumes.push_back(h);
    }
    // spdlog::warn("Got resume");
    _wait_task_signal.notify_one();
}

std::optional<runqueue::task_object> runqueue::wait_on_pending_work() {
    std::unique_lock lck(_wait_task_M);

    _wait_task_signal.wait(lck, [this]() {
        return !_pending_coro_resumes.empty() || !_pending_raw_tasks.empty() ||
               _is_dropped;
    });

    if (_is_dropped) {
        return std::nullopt;
    }

    if (!_pending_coro_resumes.empty()) {
        coro_handle h = _pending_coro_resumes.front();
        spdlog::warn("FOUND CORO");
        _pending_coro_resumes.pop_front();
        return h;
    }
    if (!_pending_raw_tasks.empty()) {
        // minimize copy
        pending_task t = _pending_raw_tasks.front();
        spdlog::warn("FOUND TASK");
        _pending_raw_tasks.pop_front();
        // TODO: change this
        return t;
    }

    return std::nullopt;
}

} // namespace async::internal
