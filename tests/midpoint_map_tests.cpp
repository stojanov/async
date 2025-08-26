#include <algorithm>
#include <async/defines.h>
#include <gtest/gtest.h>

#include <async/cont/midpoint_map.h>

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

    using mp_t = async::midpoint_map<std::string>;
    std::shared_ptr<mp_t> mp;
    order_helper o_helper;
};

using vec_t = std::vector<std::pair<async::u32, std::string>>;

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

    async::u32 co = 0;
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

        async::u32 co = 0;
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

        async::u32 co = 0;
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

TEST_F(MidPointMapTest, BIGTEST) {
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<async::u32> distrib(0, async::u32_m);

    auto create_rnd_string = [&]() {
        static std::uniform_int_distribution<async::u32> str(65, 90);
        static int count = 10;

        std::string rtn = "";

        for (int i = 0; i < count; i++) {
            rtn += ((char)(str(gen)));
        }

        return rtn;
    };

    int ready_count = 30000;
    int idle_count = 30000;

    using umap_t = std::unordered_map<async::u32, std::string>;
    using umap_it_t = umap_t::iterator;

    umap_t base;

    // by ready factor
    std::map<async::u32, umap_it_t> ready;
    // by id ready factor
    std::map<async::u32, umap_it_t> id_ready;

    /*for (int i = 0; i < ready_count; i++) {*/
    /*    auto id = distrib(gen);*/
    /**/
    /*    base[id] = create_rnd_string();*/
    /**/
    /*    auto it = base.find(id);*/
    /**/
    /*    auto factor = distrib(gen);*/
    /**/
    /*    ready[factor] = 0;*/
    /*}*/
}
