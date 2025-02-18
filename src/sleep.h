
#include <pch.h>
#include <runtime/runtime.h>

namespace async {

struct sleep_awaitable {
    sleep_awaitable(duration_t duration, int prio)
        : _prio(prio), _duration(duration) {};

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        _id = runtime::runtime::get().attach_timer(_duration, false, [h]() {
            std::cout << "------------TIMER TIMEDOUT\n";
            runtime::runtime::get().submit([h]() {
                auto _h = h;
                _h.resume();
            });
        });
    }

    void await_resume() { runtime::runtime::get().remove_timer(_id); }

    int _prio;
    duration_t _duration;
    cid_t _id;
};

inline static auto sleep(duration_t duration, int prio) {
    return async::sleep_awaitable{duration, prio};
}

} // namespace async
