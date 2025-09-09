#pragma once

#include <async/pch.h>
#include <async/runtime/runtime_core.h>
#include <async/runtime/timer_thread.h>

namespace async::internal {
class timer_thread_handler {
  public:
    timer_thread_handler(runtime_core &core) : _core(core) {}

    cid_t attach_timer(duration_t duration, bool rolling,
                       void_func on_timeout) {
        timer_thread *th = nullptr;
        u32 th_id = 0;

        {
            std::lock_guard lck(_timerM);
            if (auto *th_block = find_first_timer_thread()) {
                th = &std::get<timer_thread>(*th_block->thread_work);
                th_id = th_block->id;
                spdlog::warn("FINDING TIMER TH");
            } else {
                const auto &block = _core.spawn_new<timer_thread>();

                spdlog::warn("SPAWNING TIMER TH");

                th = &std::get<timer_thread>(*block.thread_work);
                th_id = block.id;
            }
        }

        // TODO: this has to be u32 but also the provider has to be aware of it
        // aka th
        auto timer_id = th->add_timer(duration, rolling, on_timeout);
        spdlog::warn("TIMER ADDED");

        return combine_u32(th_id, timer_id);
    }

    void remove_timer(cid_t id);

  private:
    inline runtime_core::thread_block *find_first_timer_thread() {
        auto tim = std::ranges::find_if(_core._threads, [](auto &block) {
            return std::holds_alternative<timer_thread>(*block.thread_work);
        });

        return tim == std::end(_core._threads) ? nullptr : &(*tim);
    }

    inline runtime_core::thread_block *find_timer_thread(cid_t id) {
        auto tim = std::ranges::find_if(
            _core._threads, [id](auto &block) { return block.id == id; });

        return tim == std::end(_core._threads) ? nullptr : &(*tim);
    }

  private:
    std::mutex _timerM;
    runtime_core &_core;
};
} // namespace async::internal
