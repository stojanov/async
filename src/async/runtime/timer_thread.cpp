#include "async/defines.h"
#include <async/runtime/timer_thread.h>
#include <chrono>

namespace async::internal {
timer_thread::timer_thread(duration_t resolution) : _resolution(resolution) {
    spdlog::warn(
        "TIMER THREAD CREATED {}",
        std::chrono::duration_cast<std::chrono::milliseconds>(_resolution)
            .count());
}

bool timer_thread::run_timers() {
    const auto t = clk_t::now();

    for (auto it = _timers.begin(); it != _timers.end();) {
        auto &timer = *it;

        timer.t.tick();

        if (timer.t.elapsed()) {
            spdlog::warn("Timer is finished removing");
            _id.drop(timer.id);
            it = _timers.erase(it);
            spdlog::warn("Timer is removed");
        } else {
            it++;
        }
    }

    const auto dt = clk_t::now() - t;

    if (dt > _resolution) {
        auto count = std::chrono::duration_cast<std::chrono::microseconds>(dt);
        // spdlog::warn("SATURATED dt {}\n", count.count());
        _saturated = true;
        // signal that we are overflowing
        return false;
    }

    // spdlog::warn("TIMER TICK, timer count {}\n", _timers.size());

    const auto wait_time = _resolution - dt;
    // spdlog::warn(
    //     "TIMER TIME WAIT IN MICRO {}",
    //     std::chrono::duration_cast<std::chrono::microseconds>(wait_time)
    //         .count());
    std::this_thread::sleep_for(wait_time);

    return true;
}
} // namespace async::internal
