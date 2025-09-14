
#include <async/internal/util/observer_signal.h>
#include <async/internal/util/signal_connection.h>

#include <gtest/gtest.h>

using namespace testing;

class ObserverSignalTests : public testing::Test {
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ObserverSignalTests, CanCall) {
    async::internal::utils::observer_signal<void(int)> signal;

    int k = 0;
    signal.connect([&](int i) { k = i; });

    signal(10);

    EXPECT_EQ(k, 10);
}

TEST_F(ObserverSignalTests, SignalConnectionCuts) {
    async::internal::utils::observer_signal<void(int)> signal;

    int k = 0;
    auto conn = signal.connect([&](int i) { k = i; });

    conn.disconnect();

    signal(10);

    EXPECT_EQ(k, 0);
}

TEST_F(ObserverSignalTests, ScopedSignalConnectionCuts) {
    async::internal::utils::observer_signal<void(int)> signal;

    int k = 0;

    {
        async::internal::utils::scoped_signal_connection conn =
            signal.connect([&](int i) { k = i; });
    }

    signal(10);

    EXPECT_EQ(k, 0);
}
