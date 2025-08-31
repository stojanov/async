#include <async/defines.h>
#include <async/runtime/prio_timer_thread.h>

#include <chrono>
#include <gtest/gtest.h>

using namespace testing;

class PrioTimerTests : public Test {
    void SetUp() override {}

    void TearDown() override {}

  protected:
    async::runtime::prio_timer_thread timer_th;
};

TEST_F(PrioTimerTests, TopTimer) {
    timer_th.add_timer(std::chrono::milliseconds(200), false, []() {});
    timer_th.add_timer(std::chrono::milliseconds(1000), false, []() {});
    timer_th.add_timer(std::chrono::milliseconds(2000), false, []() {});

    timer_th.print_top_timer();
}
