#include <async/runtime/runtime.h>
#include <gtest/gtest.h>

using namespace testing;

class RandomTests : public Test {
    void SetUp() override {}

    void TearDown() override {}

  protected:
    async::internal::runtime &rtime() {
        return async::internal::runtime::inst();
    }
};
