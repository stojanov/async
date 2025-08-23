#include <async/runtime/runtime.h>
#include <gtest/gtest.h>

using namespace testing;

class RandomTests : public Test {
    void SetUp() override {}

    void TearDown() override {}

  protected:
    async::runtime::runtime &rtime() { return async::runtime::runtime::get(); }
};
