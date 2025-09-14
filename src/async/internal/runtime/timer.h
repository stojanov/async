#pragma once

#include <async/internal/pch.h>

namespace async::internal {

class timer_thread;

// Base timer class, this is only meant for internal use
// provided that the callback has to be a function that usually deffers
// the heavy work to another thread in the runtime or saturates the runtime with
// work, and the callback has to be very fast when called
class timer {
    friend timer_thread;
    using resolution = std::chrono::milliseconds;

  public:
    timer(duration_t dur, bool rolling, void_func on_timeout);

    void start();
    void on_timeout(void_func &&callback);
    void release();

    duration_t duration() { return _duration; }

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
} // namespace async::internal
