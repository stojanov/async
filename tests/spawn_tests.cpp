#include <async/channel.h>
#include <async/runtime/coroutine.h>
#include <async/runtime/runtime.h>
#include <async/sleep.h>
#include <chrono>
#include <deque>
#include <gtest/gtest.h>

#include <async/poll.h>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <async/select.h>
#include <variant>

using namespace testing;

class SpawnTests : public testing::Test {
    void SetUp() override { async::runtime::runtime::get(); }

    // TODO: debug crash on shutdown
    /*void TearDown() override { async::runtime::runtime::get().shutdown(); }*/

  protected:
    async::runtime::runtime &rtime() { return async::runtime::runtime::get(); }
};

TEST_F(SpawnTests, SpawnCoroutine) {
    /*std::cout << "RAN\n";*/
    /*rtime().submit([]() { std::cout << "FROM CORO\n"; }, 1);*/
}

TEST_F(SpawnTests, SignalTests) {
    /*async::signal s;*/
    /*bool k = false;*/
    /**/
    /*rtime().submit([&]() {*/
    /*    std::cout << "A STARTED\n";*/
    /*    std::this_thread::sleep_for(std::chrono::milliseconds(400));*/
    /*    std::cout << "NOTIFIED\n";*/
    /*    k = true;*/
    /*    s.notify_all();*/
    /*    std::cout << "A FINISHED\n";*/
    /*});*/
    /**/
    /*rtime().submit([&]() -> async::runtime::coroutine {*/
    /*    std::cout << "B STARTED\n";*/
    /*    co_await s.wait_if([&]() { return k; });*/
    /*    std::cout << "B FINISHED\n";*/
    /*});*/
}

/*TEST_F(SpawnTests, ChannelTests) {*/
/*    async::channel<int> chan;*/
/**/
/*    rtime().submit([&chan]() -> async::runtime::coroutine {*/
/*        auto &chan_ref = chan;*/
/**/
/*        int i = 0;*/
/*        while (1) {*/
/*            std::cout << "PUSHING " << i << "\n";*/
/*            chan_ref.push(i++);*/
/**/
/*            // TODO: add sleep*/
/*            std::this_thread::sleep_for(std::chrono::milliseconds(500));*/
/*        }*/
/*    });*/
/**/
/*    rtime().submit([&chan]() -> async::runtime::coroutine {*/
/*        auto &chan_ref = chan;*/
/**/
/*        while (1) {*/
/*            std::cout << "TRYING FETCH from A" << "\n";*/
/**/
/*            auto v = co_await chan_ref.fetch();*/
/**/
/*            std::cout << "FETCHED CHANNEL A" << v << "\n";*/
/*        }*/
/*    });*/
/**/
/*    while (1) {*/
/*        std::this_thread::sleep_for(std::chrono::milliseconds(100));*/
/*    }*/
/*}*/

/*TEST_F(SpawnTests, SleepTests) {*/
/*    rtime().submit([]() -> async::runtime::coroutine {*/
/*        while (1) {*/
/*            std::cout << "GOING TO SLEEP" << "\n";*/
/**/
/*            co_await async::sleep(std::chrono::milliseconds(500), 1);*/
/**/
/*            std::cout << "UNSLEPT \n";*/
/*        }*/
/*    });*/
/**/
/*    while (1) {*/
/*        std::this_thread::sleep_for(std::chrono::milliseconds(100));*/
/*    }*/
/*}*/

template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

TEST_F(SpawnTests, ChannelTests) {
    std::shared_ptr<async::channel<int>> chan =
        std::make_shared<async::channel<int>>();

    std::shared_ptr<async::channel<std::string>> chan1 =
        std::make_shared<async::channel<std::string>>();

    std::shared_ptr<async::channel<char>> chan2 =
        std::make_shared<async::channel<char>>();

    spdlog::warn("IDS {} {} {}", chan->id(), chan1->id(), chan2->id());

    async::co_select sl{chan, chan1, chan2};

    std::mutex m;
    std::deque<decltype(sl)::variant_type> queue;
    std::atomic_bool running = true;

    rtime().submit([&sl, &m, &queue, &running]() -> async::runtime::coroutine {
        auto &select = sl;
        auto &mu = m;
        auto &q = queue;
        auto &r = running;

        overloads visitor{[&mu, &q](int i) {
                              std::lock_guard lck(mu);
                              auto k = q.front();
                              q.pop_front();

                              EXPECT_TRUE(std::holds_alternative<int>(k));
                              std::cout << "GOT INTEGER " << i << "\n";
                          },
                          [&mu, &q](std::string &str) {
                              std::lock_guard lck(mu);
                              auto k = q.front();
                              q.pop_front();
                              EXPECT_TRUE(
                                  std::holds_alternative<std::string>(k));
                              std::cout << "GOT STRING " << str << "\n";
                          },
                          [&mu, &q](char i) {
                              std::lock_guard lck(mu);
                              auto k = q.front();
                              q.pop_front();
                              EXPECT_TRUE(std::holds_alternative<char>(k));
                              std::cout << "GOT FLOAT " << i << "\n";
                          }};

        spdlog::error("STARTED COLLECTING FROM SELECT BEFORE POLL");
        while (co_await async::poll()) {
            std::cout << "-------STARTED WAITING--------" << "\n";
            auto v = co_await select.fetch();

            std::cout << "SELECT FINISHED \n";
            if (v) {
                std::visit(visitor, *v);
            } else {
            }
        }
    });

    rtime().submit(
        [&chan, &m, &queue, &running]() -> async::runtime::coroutine {
            auto &chan_ref = chan;
            auto &mu = m;
            auto &q = queue;
            auto &r = running;

            int i = 0;
            spdlog::error("STARTED PUSHING INT FROM SELECT BEFORE POLL");
            while (co_await async::poll()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));

                std::cout << "-------- PUSHING INT " << i << "\n";
                chan_ref->push(i);
                {
                    std::lock_guard lck(mu);
                    q.push_back(i);
                }
                i++;
            }
        });

    rtime().submit(
        [&chan1, &m, &queue, &running]() -> async::runtime::coroutine {
            auto &chan_ref = chan1;
            auto &mu = m;
            auto &q = queue;
            auto &r = running;

            std::string str = "CHAN SEND ";
            int i = 0;
            spdlog::error("STARTED PUSHING \tSTRING FROM SELECT BEFORE POLL");
            while (co_await async::poll()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                auto tosend = str + std::to_string(i++);
                std::cout << "-------- PUSHING STRING\n";

                chan_ref->push(tosend);

                {

                    std::lock_guard lck(mu);
                    q.push_back(tosend);
                }

                // TODO: add sleep
            }
        });

    rtime().submit(
        [&chan2, &queue, &m, &running]() -> async::runtime::coroutine {
            auto &chan_ref = chan2;
            auto &mu = m;
            auto &q = queue;
            auto &r = running;

            char c = 'a';
            spdlog::error("STARTED PUSHING \tCHAR FROM SELECT BEFORE POLL");
            while (co_await async::poll()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));

                std::cout << "-------- PUSHING CHAR " << c << "\n";

                chan_ref->push(c);

                {
                    std::lock_guard lck(mu);
                    q.push_back(c);
                }

                c++;
                // TODO: add sleep
            }
        });

    std::this_thread::sleep_for(std::chrono::seconds(200));

    running = false;
    rtime().shutdown();
}
