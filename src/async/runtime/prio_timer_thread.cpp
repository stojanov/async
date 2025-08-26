#include <async/runtime/prio_timer_thread.h>

namespace async::runtime {
void prio_timer_thread::run_timers() {

    while (_running) {
        std::unique_lock lck(_timerM);

        if (_prio_map.empty()) {
            _timerCV.wait(lck,
                          [this] { return !_running || !_prio_map.empty(); });
        }

        auto latest_entry = _prio_map.end();
        std::advance(latest_entry, -1);

        if (latest_entry->first > clk_t::now()) {
            if (auto i = _block_map.find(latest_entry->second);
                i != std::end(_block_map)) {
                i->second.timeout();
            }
            _prio_map.erase(latest_entry);
        } else {
            const auto current_timer_count = _prio_map.size();
            _timerCV.wait_until(
                lck, latest_entry->first, [this, current_timer_count] {
                    return !_running || current_timer_count != _prio_map.size();
                });
        }
    }
}
} // namespace async::runtime
