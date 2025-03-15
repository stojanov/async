#include <atomic>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#include "async/defines.h"
#include "async/runtime/coroutine.h"
#include <async/poll.h>
#include <async/runtime/runtime.h>
#include <chrono>
#include <gtest/gtest.h>

using namespace testing;

class PollTests : public Test {
    void SetUp() override {}

    void TearDown() override {}

  protected:
    async::runtime::runtime &rtime() { return async::runtime::runtime::get(); }
};

TEST_F(PollTests, PerfTest) {
    async::clk_t::duration dur;
    async::clk_t::duration max;
    async::clk_t::duration min;

    auto last = async::clk_t::now();

    std::atomic_size_t c = 0;
    bool running = true;

    rtime().submit([&] -> async::runtime::coroutine {
        auto &co = c;
        auto &l = last;
        auto &d = dur;
        auto &r = running;

        while (co_await async::poll() && r) {
            /*std::cout << "|n";*/
            co++;
        }
    });

    rtime().submit([&] -> async::runtime::coroutine {
        auto &co = c;
        auto &l = last;
        auto &d = dur;
        auto &r = running;

        while (co_await async::poll() && r) {
            /*std::cout << "|n";*/
            co++;
        }
    });

    rtime().submit([&] -> async::runtime::coroutine {
        auto &co = c;
        auto &l = last;
        auto &d = dur;
        auto &r = running;

        while (co_await async::poll() && r) {
            /*std::cout << "|n";*/
            co++;
        }
    });

    rtime().submit([&] -> async::runtime::coroutine {
        auto &co = c;
        auto &l = last;
        auto &d = dur;
        auto &r = running;

        while (co_await async::poll() && r) {
            /*std::cout << "|n";*/
            co++;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    running = false;
    auto avg =
        std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() / c *
        1.f;

    auto manual = 1 * 1.f / c;

    spdlog::warn("AVERAGE TIME BETWEEN POLLS {}, Switch count {}, manual {}",
                 avg, c.load(), manual);
}
