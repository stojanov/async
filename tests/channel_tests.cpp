
#include "async/utils.h"
#include <async/channel.h>
#include <async/runtime/runtime.h>
#include <async/runtime/task_handle.h>
#include <async/select.h>
#include <async/sleep.h>
#include <chrono>
#include <deque>
#include <gtest/gtest.h>
#include <new>
#include <regex.h>

using namespace testing;

class ChannelTests : public testing::Test {
    void SetUp() override {}

    void TearDown() override {
        async::internal::runtime::public_inst().shutdown();
    }
};

TEST_F(ChannelTests, ChannelTest) {
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

    spdlog::warn("WAIT PRODUCER");
    producer_handle->thread().join();
    spdlog::warn("WAIT CONSUMER");
    consumer_handle->thread().join();

    EXPECT_EQ(recieved.size(), MAX_COUNT);
    int counter = 0;
    for (auto i : recieved) {
        EXPECT_EQ(i, counter);
        counter++;
    }
}

TEST_F(ChannelTests, SelectTest) {
    auto int_channel = std::make_shared<async::channel<int>>();
    auto string_channel = std::make_shared<async::channel<std::string>>();

    std::vector<std::variant<int, std::string>> received;
    std::vector<std::variant<int, std::string>> sent;

    enum Order { Int, String };

    std::mutex sentM;
    std::vector<Order> order;
    int MAX_COUNT = 10000;

    auto producer1 = async::runtime::submit([&] -> async::coroutine<> {
        int counter = 0;

        for (int counter = 0; counter < MAX_COUNT; counter++) {
            std::lock_guard lck(sentM);
            int_channel->push(counter);
            sent.push_back(counter);
        }

        co_return;
    });

    auto producer2 = async::runtime::submit([&] -> async::coroutine<> {
        std::string out = "OUT ";

        int counter = 0;

        for (int counter = 0; counter < MAX_COUNT; counter++) {
            std::lock_guard lck(sentM);
            std::string res = out + std::to_string(counter);
            string_channel->push(res);
            sent.push_back(counter);
        }

        co_return;
    });

    auto consumer = async::runtime::submit([&] -> async::coroutine<> {
        auto end = MAX_COUNT * 2;

        int counter = 0;

        async::co_select sel(int_channel, string_channel);

        while (true) {
            if (counter == end) {
                co_return;
            }

            auto value = co_await sel.fetch();

            auto visitor = async::internal::var_overload{
                [&](int v) { received.push_back(v); },
                [&](std::string v) { received.push_back(v); }};

            if (value) {
                std::visit(visitor, *value);
            } else {
                co_return;
            }

            counter++;
        }

        co_return;
    });

    spdlog::warn("WAIT FIRST");
    producer1->thread().join();
    spdlog::warn("WAIT SECOND");
    producer2->thread().join();
    spdlog::warn("WAIT THIRD");
    consumer->thread().join();

    EXPECT_EQ(received.size(), sent.size());

    for (int i = 0; i < received.size(); i++) {
        auto recieved_value = received[i];
        auto sent_value = sent[i];

        // EXPECT_EQ(recieved_value, sent_value);
    }
    // int counter = 0;
    // for (auto &v : received) {
    //     std::string got_string;
    //     int got_int;
    //
    //     Order o;
    //     auto received_visitor = async::internal::var_overload{
    //         [&](int v) {
    //             got_int = v;
    //             o = Int;
    //         },
    //         [&](std::string v) {
    //             got_string = v;
    //             o = String;
    //         },
    //     };
    //
    //     std::visit(received_visitor, v);
    //
    //     EXPECT_EQ(o, order[counter]);
    //
    //     if (o == Order::Int == order[counter]) {
    //         EXPECT_EQ(got_int, std::get<int>(v));
    //     }
    //     if (o == Order::String == order[counter]) {
    //         EXPECT_EQ(got_string, std::get<std::string>(v));
    //     }
    //
    //     counter++;
    // }
}
