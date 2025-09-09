#include <async/runtime/defines.h>
#include <async/runtime/runqueue.h>
#include <coroutine>

namespace async::internal {
runqueue::runqueue() {}

// void runqueue::push_pending_raw_task(task_block &&block) {
//     {
//         std::unique_lock lck(_raw_pending_M);
//         _pending_raw_tasks.push_back(std::move(block));
//     }
//
//     spdlog::warn("Got task");
//     _raw_pending_signal.notify_one();
//     _wait_task_signal.notify_one();
// }

void runqueue::push_pending_resume(std::coroutine_handle<> h) {
    // should be locked
    {
        std::lock_guard lck(_pending_coro_res_M);
        _pending_coro_resumes.push_back(h);
    }
    // spdlog::warn("Got resume");
    _wait_task_signal.notify_one();
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
            coro_handle h = _pending_coro_resumes.back();
            // spdlog::warn("FOUND CORO");
            _pending_coro_resumes.pop_back();
            return h;
        }
    }

    {
        std::lock_guard lck(_raw_pending_M);
        if (!_pending_raw_tasks.empty()) {
            // minimize copy
            task_package t = _pending_raw_tasks.back();
            spdlog::warn("FOUND TASK");
            _pending_raw_tasks.pop_back();
            return t;
        }
    }

    return std::nullopt;
}

} // namespace async::internal
