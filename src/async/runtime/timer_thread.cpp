#include "async/defines.h"
#include <async/runtime/timer_thread.h>
#include <chrono>

namespace async::runtime {
timer_thread::timer_thread(duration_t resolution) : _resolution(resolution) {}

bool timer_thread::work() {
    const auto t = clk_t::now();

    for (auto it = _timers.begin(); it != _timers.end();) {
        auto &timer = *it;

        timer.t.tick();

        if (timer.t.finished()) {
            it = _timers.erase(it);
        }
    }

    const auto dt = clk_t::now() - t;

    if (dt > _resolution) {
        _saturated = true;
        // signal that we are overflowing
        return false;
    }

    const auto wait_time = _resolution - dt;
    std::this_thread::sleep_for(wait_time);

    return true;
}
} // namespace async::runtime
