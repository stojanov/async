
#include <async/pch.h>
#include <async/runtime/runtime.h>

namespace async {

namespace internal {

struct sleep_awaitable {
    sleep_awaitable(duration_t duration, int prio)
        : _prio(prio), _duration(duration) {};

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        _id = internal::runtime::inst().attach_timer(_duration, false, [h]() {
            internal::runtime::inst().submit_resume(h);
        });
    }

    void await_resume() {}

    int _prio;
    duration_t _duration;
    cid_t _id;
};
} // namespace internal

inline static internal::sleep_awaitable sleep(duration_t duration,
                                              int prio = 1) {
    return {duration, prio};
}

} // namespace async
