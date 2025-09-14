#include <async/internal/runtime/timer.h>

namespace async::internal {
timer::timer(duration_t s, bool rolling, void_func on_timeout)
    : _duration(std::chrono::duration_cast<std::chrono::milliseconds>(s)),
      _on_timeout(on_timeout) {}

void timer::start() { _last_start = clk_t::now(); }

void timer::release() { _on_timeout(); }

void timer::tick() {
    if (_finished || (!_rolling && _over_step)) {
        _finished = true;
        return;
    }

    const auto now = clk_t::now();
    const auto took = now - _last_start;

    if (took > _duration) {
        spdlog::warn("TIMER FINISHED\n");
        _over_step = true;
        _finished = true;

        _on_timeout();

        spdlog::warn("AFTER FUNC");

        _last_start = now;
    } else {
        if (_over_step) {
            _over_step = false;
        }
    }
}
} // namespace async::internal
