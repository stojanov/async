#include <gtest/gtest.h>

#include <async/internal/cont/interval_map.h>
#include <optional>

using namespace testing;
using namespace async;

using mp_t = async::internal::interval_map<std::uint64_t, std::string>;

class IntervalMapTests : public testing::Test {
    void SetUp() override { _mp = std::make_shared<mp_t>(); }

    void TearDown() override { _mp.reset(); }

  protected:
    std::shared_ptr<mp_t> _mp;
};

TEST_F(IntervalMapTests, GenericTest) {}

TEST_F(IntervalMapTests, TestBasicQuery) {
    _mp->add_interval({1, 10}, "A");
    _mp->add_interval({2, 4}, "B");
    _mp->add_interval({6, 8}, "C");
    _mp->add_interval({0, 12}, "D");

    _mp->print();

    auto a1 = _mp->find_in_interval(1);
    auto b = _mp->find_in_interval(3);
    auto a2 = _mp->find_in_interval(5);
    auto c = _mp->find_in_interval(6);
    auto a3 = _mp->find_in_interval(9);
    auto a10 = _mp->find_in_interval(20);
    auto d1 = _mp->find_in_interval(0);
    auto d2 = _mp->find_in_interval(11);

    EXPECT_NE(a1, std::nullopt);
    EXPECT_NE(a2, std::nullopt);
    EXPECT_NE(a3, std::nullopt);

    EXPECT_NE(b, std::nullopt);
    EXPECT_NE(c, std::nullopt);
    EXPECT_NE(c, std::nullopt);

    EXPECT_NE(d1, std::nullopt);
    EXPECT_NE(d2, std::nullopt);

    std::string aS = "A";
    std::string bS = "B";
    std::string cS = "C";
    std::string dS = "D";

    EXPECT_EQ(**a1, aS);
    EXPECT_EQ(**a2, aS);
    EXPECT_EQ(**a3, aS);

    EXPECT_EQ(**b, bS);
    EXPECT_EQ(**c, cS);

    EXPECT_EQ(**d1, dS);
    EXPECT_EQ(**d2, dS);

    EXPECT_EQ(a10, std::nullopt);
}

TEST_F(IntervalMapTests, Overlap) {
    _mp->add_interval_overlap_filler_clear({1, 20}, "A");
    _mp->add_interval_overlap_filler_clear({5, 8}, "B");
    _mp->add_interval_overlap_filler_clear({12, 15}, "C");
    _mp->add_interval_overlap_filler_clear({0, 22}, "D");
    _mp->add_interval_overlap_filler_clear({6, 13}, "E");

#define TEST_STR(o, s)                                                         \
    do {                                                                       \
        EXPECT_NE(o, std::nullopt);                                            \
        EXPECT_EQ(**(o), (s));                                                 \
    } while (0)

    _mp->print();

    auto a1 = _mp->find_in_interval(2);
    auto e2 = _mp->find_in_interval(10);
    auto d1 = _mp->find_in_interval(21);
    auto b1 = _mp->find_in_interval(5);
    auto e1 = _mp->find_in_interval(6);

    TEST_STR(a1, "A");
    TEST_STR(e2, "E");
    TEST_STR(d1, "D");
    TEST_STR(b1, "B");
    TEST_STR(e1, "E");
}
