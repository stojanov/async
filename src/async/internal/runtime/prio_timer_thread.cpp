#include <async/internal/runtime/prio_timer_thread.h>

namespace async::internal {
void prio_timer_thread::run_timers() {
    while (_running) {
        std::unique_lock lck(_timerM);

        if (_prio_queue.empty()) {
            _timerCV.wait(lck,
                          [this] { return !_running || !_prio_queue.empty(); });
        }

        // latest as in latest to fire
        const auto &latest_entry = _prio_queue.top();

        if (latest_entry.fire_point > clk_t::now()) {
            if (auto i = _block_map.find(latest_entry.id);
                i != std::end(_block_map)) {
                i->second.timeout();
            }
            _prio_queue.pop();
        } else {
            const auto current_timer_count = _prio_queue.size();
            _timerCV.wait_until(lck, latest_entry.fire_point,
                                [this, current_timer_count] {
                                    return !_running || current_timer_count !=
                                                            _prio_queue.size();
                                });
        }
    }
}
} // namespace async::internal
