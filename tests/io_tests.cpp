#include <async/io/base.h>
#include <async/read.h>
#include <async/runtime/runtime.h>
#include <gtest/gtest.h>
#include <unistd.h>

using namespace testing;

class IOTests : public Test {
    void SetUp() override {}

    void TearDown() override {}

  protected:
    async::internal::runtime &rtime() {
        return async::internal::runtime::inst();
    }
};

TEST_F(IOTests, ReadSTDIN) {

    rtime().submit_coro([]() -> async::coroutine<> {
        auto fd = async::io::io_handle(STDIN_FILENO);

        std::vector<std::byte> vector;

        vector.resize(1024);

        while (int n = co_await async::read(fd, vector, 1024)) {
            std::cout << "REPLY ";
            for (int i = 0; i < n; i++) {
                std::cout << (char)vector[i];
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(10000));
}
