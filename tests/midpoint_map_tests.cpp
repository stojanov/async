#include <cstdint>
#include <gtest/gtest.h>

#include <async/internal/cont/midpoint_map.h>

#include <algorithm>
#include <random>

class order_helper {
  public:
    struct entry {
        int key;
        int factor;
        std::string value;
    };

    entry find_key(int key) {}

    entry find_first_factor(int factor) {}

    entry find_first_value(std::string &value) {}

  private:
};

class MidPointMapTest : public testing::Test {
  protected:
    struct entry {
        int key;
        int factor;
        std::string value;
    };

    void SetUp() { mp = std::make_shared<mp_t>(); }

    void TearDown() { mp.reset(); }

  protected:
    void fill_dummy() {
        mp->add(1, "A", 1);
        mp->add(2, "B", 0);
        mp->add(3, "C", 1);
        mp->add(4, "D", 0);
        mp->add(9, "E", 2);
        mp->add(10, "X", 3);
        mp->add(11, "K", 4);
        mp->add(12, "K1", 2);
        mp->add(6, "F", 1);
        mp->add(7, "G", 1);
        mp->add(8, "H", 1);
    }

    void fill_dummy_reverse() {
        mp->add(1, "A", async::internal::u32_m - 1);
        mp->add(2, "B", 0);
        mp->add(3, "C", async::internal::u32_m - 1);
        mp->add(4, "D", 0);
        mp->add(9, "E", async::internal::u32_m - 2);
        mp->add(10, "X", async::internal::u32_m - 3);
        mp->add(11, "K", async::internal::u32_m - 4);
        mp->add(12, "K1", async::internal::u32_m - 2);
        mp->add(6, "F", async::internal::u32_m - 1);
        mp->add(7, "G", async::internal::u32_m - 1);
        mp->add(8, "H", async::internal::u32_m - 1);
    }

    using mp_t = async::internal::midpoint_map<std::string>;
    std::shared_ptr<mp_t> mp;
    order_helper o_helper;
};

using vec_t = std::vector<std::pair<std::uint64_t, std::string>>;

TEST_F(MidPointMapTest, PRINTMAP) {
    fill_dummy();
    mp->print("MIDPOINTMAP TEST");
}

TEST_F(MidPointMapTest, EnsureSortingIsMaintainedSamePrio) {
    fill_dummy();

    vec_t expected = {
        {1, "A"},   {3, "C"},  {6, "F"},  {7, "G"}, {8, "H"}, {9, "E"},
        {12, "K1"}, {10, "X"}, {11, "K"}, {2, "B"}, {4, "D"},
    };

    std::uint32_t co = 0;
    for (auto i = mp->value_key_begin(); i != mp->value_key_end(); i++) {
        auto &k = expected[co];
        auto vi = *i;

        EXPECT_EQ(vi.first, k.first);
        EXPECT_EQ(*vi.second, k.second);

        co++;
    }
}

TEST_F(MidPointMapTest, EnsureSortingIsMaintainedSamePrio_UpdateTest) {
    fill_dummy();

    {
        mp->update_prio(7, 3);

        vec_t expected = {{1, "A"},  {3, "C"},   {6, "F"}, {8, "H"},
                          {9, "E"},  {12, "K1"}, {7, "G"}, {10, "X"},
                          {11, "K"}, {2, "B"},   {4, "D"}};

        std::uint32_t co = 0;
        for (auto i = mp->value_key_begin(); i != mp->value_key_end(); i++) {
            auto &k = expected[co];
            auto vi = *i;

            EXPECT_EQ(vi.first, k.first);
            EXPECT_EQ(*vi.second, k.second);

            co++;
        }
    }

    {
        mp->update_prio(2, 4);

        vec_t expected = {{1, "A"}, {3, "C"},   {6, "F"}, {8, "H"},
                          {9, "E"}, {12, "K1"}, {7, "G"}, {10, "X"},
                          {2, "B"}, {11, "K"},  {4, "D"}};

        std::uint32_t co = 0;
        for (auto i = mp->value_key_begin(); i != mp->value_key_end(); i++) {
            auto &k = expected[co];
            auto vi = *i;

            EXPECT_EQ(vi.first, k.first);
            EXPECT_EQ(*vi.second, k.second);

            co++;
        }
    }
}

TEST_F(MidPointMapTest, EnsureCanFindByPrio) {
    fill_dummy();

    {
        const auto [start, end] = mp->find_prio(1);
        const auto end_value = *end;

        EXPECT_EQ(start->second, "A");
        EXPECT_EQ(end_value->second, "E");
    }

    {
        const auto [start, end] = mp->find_prio(2);
        const auto end_value = *end;

        EXPECT_EQ(start->second, "E");
        EXPECT_EQ(end_value->second, "X");
    }

    {
        const auto [start, end] = mp->find_prio(4);

        EXPECT_EQ(start->second, "K");
        EXPECT_EQ(end, std::nullopt);
    }

    {
        const auto [start, end] = mp->find_prio(0);

        EXPECT_EQ(start->second, "B");
        EXPECT_EQ(end, mp->end());
    }
}

TEST_F(MidPointMapTest, IncreasePrioTestsByU32Max) {
    fill_dummy_reverse();

    mp->modify_prio_by_factor(11, -12);
    mp->print("TEST REVERSE MAP");
}
