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
    async::internal::runtime &rtime() {
        return async::internal::runtime::inst();
    }
};

TEST_F(PollTests, PerfTest) {
    async::duration_t dur;
    async::duration_t max;
    async::duration_t min;

    auto last = async::clk_t::now();

    std::atomic_size_t c = 0;
    bool running = true;

    int coro_count = 4;

    for (int i = 0; i < coro_count; i++) {
        rtime().submit_coro([&] -> async::coroutine {
            auto &co = c;
            auto &l = last;
            auto &d = dur;
            auto &r = running;

            while (co_await async::timed_poll(std::chrono::milliseconds(20)) &&
                   r) {
                /*std::cout << "|n";*/
                co++;
            }

            spdlog::warn("CORO FINISHED FROM FUNCTION OBJECT");
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    running = false;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    dur = async::clk_t::now() - last;

    auto time =
        std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

    auto avg = time * 1.f / c;

    auto manual = 1 * 1.f / c;
    spdlog::warn("Coro count: {}\tTime: {}ms\tAverage time between poll: "
                 "{}ms\tSwitch Count: {}",
                 coro_count, time, avg, c.load());
}
