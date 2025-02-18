#include <chrono>
#include <runtime/timer.h>

namespace async::runtime {
timer::timer(std::chrono::seconds s, bool rolling) : _duration(s) {}

timer::timer(std::chrono::milliseconds ms, bool rolling) : _duration(ms) {}

void timer::start() { _last_start = clk_t::now(); }

void timer::on_timeout(void_func &&callback) {
    _on_timeout = std::move(callback);
}

void timer::release() {
    if (_on_timeout) {
        _on_timeout();
    }
}

void timer::tick() {
    if (_finished || (!_rolling && _over_step)) {
        _finished = true;
        return;
    }

    const auto now = clk_t::now();
    const auto took = clk_t::now() - _last_start;

    if (took > _duration) {
        _over_step = true;

        if (_on_timeout) {
            // hmm ?
            // schedule if takes time
            _on_timeout();
        }

        _last_start = now;
    } else {
        if (_over_step) {
            _over_step = false;
        }
    }
}
} // namespace async::runtime
