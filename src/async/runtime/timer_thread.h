#pragma once

#include <async/defines.h>
#include <async/pch.h>

#include <async/runtime/timer.h>

#include <iostream>

namespace async::runtime {
class timer_thread {
  private:
    struct timer_block {
        timer t;
        cid_t id;
    };

  public:
    timer_thread(duration_t resolution = duration_t(1));

    bool work();

    cid_t add_timer(duration_t duration, bool rolling, void_func on_timeout) {
        const auto id = _id.get();

        auto new_timer = timer(duration, rolling);
        new_timer.on_timeout(std::move(on_timeout));
        new_timer.start();

        std::lock_guard lck(_timers_m);
        _timers.emplace_back(std::move(new_timer), id);
        std::cout << "\tADDED TIMER " << id << "\n";

        return id;
    }

    void remove_timer(cid_t id) {
        std::lock_guard lck(_timers_m);

        // to avoid look up, hold a bitset of all enabled timers on this thread
        // even if you do a look up do it on a sorted list, with binary search
        // deletion should be log N in case of sorted list

        auto erased =
            std::erase_if(_timers, [id](auto &tim) { return tim.id == id; });

        if (erased >= 1) {
            std::cout << "\tREMOVED TIMER " << id << "\n";
            _id.drop(id);
        }
    }

    // TODO:
    [[nodiscard]] bool saturated() { return _saturated; }

    void cleanup() {
        {
            std::lock_guard lck(_timers_m);

            _timers.clear();
        }
        _id.reset();
    }

  private:
    std::chrono::milliseconds _resolution;

    std::atomic_bool _saturated{false};

    // operations queue, add/erase
    // process on work()
    std::mutex _timers_m;
    // TODO: this should use the sparse set list
    std::list<timer_block> _timers;
    id_gen<cid_t> _id;
};

using p_timer_thread = std::unique_ptr<timer_thread>;
} // namespace async::runtime
