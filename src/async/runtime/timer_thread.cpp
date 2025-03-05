#include <async/runtime/timer_thread.h>

namespace async::runtime {
timer_thread::timer_thread(std::chrono::milliseconds resolution) {}

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
        return false;
    }

    const auto wait_time = _resolution - dt;
    std::this_thread::sleep_for(wait_time);

    return true;
}
} // namespace async::runtime
