#include <async/channel.h>
#include <async/defines.h>
#include <async/runtime/coroutine.h>
#include <async/runtime/runtime.h>
#include <async/runtime/task_handle.h>
#include <async/sleep.h>
#include <chrono>
#include <deque>
#include <gtest/gtest.h>

#include <async/poll.h>
#include <iostream>
#include <ratio>
#include <string>
#include <thread>
#include <unistd.h>

#include <async/select.h>
#include <async/signal.h>
#include <variant>

using namespace testing;

class SpawnTests : public testing::Test {
    void SetUp() override { rtime().inst(); }

    // TODO: debug crash on shutdown
    /*void TearDown() override { async::runtime::runtime::get().shutdown(); }*/

  protected:
    async::internal::runtime &rtime() {
        return async::internal::runtime::inst();
    }
};

TEST_F(SpawnTests, SpawnCoroutineAndGetResult) {
    auto h =
        async::runtime::submit([]() -> async::coroutine<int> { co_return 42; });

    auto h2 = async::runtime::submit([]() -> async::coroutine<int> {
        auto h = async::runtime::submit(
            []() -> async::coroutine<int> { co_return 24; });

        auto res = co_await h->coro().result_coro();
        co_return res;
    });

    EXPECT_EQ(h->thread().result(), 42);
    EXPECT_EQ(h2->thread().result(), 24);
}

TEST_F(SpawnTests, SignalTests) {
    async::signal s;

    bool notified = false;

    int state = 0;

    auto handle = async::runtime::submit([&]() -> async::coroutine<> {
        state = 1;
        co_await s.wait_if([&] { return notified; });
        state = 2;
    });

    notified = true;
    s.notify_all();

    handle->thread().join();

    EXPECT_EQ(state, 2);
}

TEST_F(SpawnTests, ChannelTests) {
    async::channel<int> chan;
    std::vector<int> recieved;

    bool ready;
    int MAX_COUNT = 10000;

    auto producer_handle = async::runtime::submit([&] -> async::coroutine<> {
        int counter = 0;

        for (int counter = 0; counter < MAX_COUNT; counter++) {
            chan.push(counter);
        }

        co_return;
    });

    auto consumer_handle = async::runtime::submit([&] -> async::coroutine<> {
        while (true) {
            auto res = co_await chan.fetch();

            if (res) {
                recieved.push_back(*res);

                if (*res == (MAX_COUNT - 1)) {
                    co_return;
                }
            } else {
                co_return;
            }
        }
        co_return;
    });

    producer_handle->thread().join();
    consumer_handle->thread().join();

    EXPECT_EQ(recieved.size(), MAX_COUNT);
    int counter = 0;
    for (auto i : recieved) {
        EXPECT_EQ(i, counter);
        counter++;
    }
}
