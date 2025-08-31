#pragma once

#include <async/cont/midpoint_map.h>
#include <async/defines.h>
#include <async/runtime/timer.h>

namespace async::runtime {

class prio_timer_thread {
    struct timer_block {
        // for future, is a struct
        void_func timeout;
    };

    struct prio_timer {
        clk_t::time_point fire_point;
        u32 id;

        bool operator<(const prio_timer &other) const {
            return fire_point < other.fire_point;
        }

        bool operator>(const prio_timer &other) const {
            return fire_point > other.fire_point;
        }
    };

  public:
    u32 add_timer(duration_t duration, bool rolling, void_func on_timeout) {
        const auto id = _ids.get();

        clk_t::time_point fire_point = clk_t::now() + duration;

        {
            std::lock_guard _lck(_timerM);
            _block_map.insert(std::pair(id, timer_block{on_timeout}));
            _prio_queue.emplace(fire_point, id);
        }
        _timerCV.notify_all();

        auto milis =
            std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                .count();

        spdlog::warn("ADDED TIMER ID {}, millis {}", id, milis);

        return id;
    }

    void remove_timer(u32 id) {
        std::lock_guard _lck(_timerM);
        _block_map.erase(id);
    }

    void signal_shutdown() {
        _running = false;
        _timerCV.notify_all();
    }

    void run_timers();

    void print_top_timer() {
        auto &t = _prio_queue.top();
        spdlog::warn("top timer id {}", t.id);
    }

  private:
    id_gen<u32> _ids;
    std::atomic_bool _running{true};
    std::mutex _timerM;
    std::condition_variable _timerCV;
    std::map<u32, timer_block> _block_map;
    // maybe vector isn't best
    std::priority_queue<prio_timer, std::vector<prio_timer>,
                        std::greater<prio_timer>>
        _prio_queue;
};
} // namespace async::runtime
