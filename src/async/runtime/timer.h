#pragma once

#include "async/defines.h"
#include <async/pch.h>

namespace async::runtime {

class timer_thread;

class timer {
    friend timer_thread;
    using resolution = std::chrono::milliseconds;

  public:
    timer(duration_t dur, bool rolling);

    void start();
    void on_timeout(void_func &&callback);
    void release();

    [[nodiscard]] bool elapsed() const { return _over_step; }
    [[nodiscard]] bool rolling() const { return _rolling; }
    [[nodiscard]] bool finished() const { return _finished; }

  private:
    void tick();

  private:
    resolution _duration;

    clk_t::time_point _last_start;

    void_func _on_timeout{nullptr};

    bool _rolling{false};
    bool _over_step{false};
    bool _finished{false};
};
} // namespace async::runtime
