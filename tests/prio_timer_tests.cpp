#include <async/defines.h>
#include <async/new_select.h>
#include <async/runtime/prio_timer_thread.h>
#include <async/runtime/runtime.h>
#include <async/runtime/task_handle.h>
#include <async/time.h>

#include <chrono>
#include <gtest/gtest.h>

using namespace testing;

class PrioTimerTests : public Test {
    void SetUp() override {}

    void TearDown() override {
        async::internal::runtime::public_inst().shutdown();
    }

  protected:
};

TEST_F(PrioTimerTests, TopTimer) {

    auto h = async::runtime::submit([]() -> async::coroutine<> {
        async::timer t = async::create_timer(std::chrono::milliseconds(2000));
        async::timer t2 = async::create_timer(std::chrono::milliseconds(4000));

        for (int i = 0; i < 2; i++) {
            auto match_select = co_await async::select_test(t, t2);

            match_select.on(t, []() {
                spdlog::error("---------------------- T FINISHED");
            });
            match_select.on(
                t2, []() { spdlog::error("------------------- T2 FINISHED"); });
        }

        co_return;
    });

    h->thread().join();
}
