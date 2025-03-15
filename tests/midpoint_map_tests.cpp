#include <async/defines.h>
#include <gtest/gtest.h>

#include <async/cont/midpoint_map.h>

#include <random>

class MidPointMapTest : public testing::Test {

    void SetUp() { mp = std::make_shared<mp_t>(); }

    void TearDown() { mp.reset(); }

  protected:
    using mp_t = async::midpoint_map<std::string>;
    std::shared_ptr<mp_t> mp;
};

using vec_t = std::vector<std::pair<async::u32, std::string>>;

TEST_F(MidPointMapTest, EnsureSortingIsMaintained) {
    mp->add(1, "A", 10);
    mp->add(2, "B", 0);
    mp->add(3, "C", 0);
    mp->add(4, "D", 0);
    mp->add(5, "E", 1);

    vec_t expected = {
        {5, "E"}, {1, "A"}, {0, ""}, {2, "B"}, {3, "C"}, {4, "D"},
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

TEST_F(MidPointMapTest, EnsureCanFindByPrio) {
    mp->add(1, "A", 1);
    mp->add(2, "B", 0);
    mp->add(3, "C", 1);
    mp->add(4, "D", 0);
    mp->add(5, "E", 2);
    mp->add(6, "F", 1);
    mp->add(7, "G", 1);
    mp->add(8, "H", 1);

    mp->print("My map");

    auto k = mp->find_prio(1);

    std::cout << "VAL START " << (k.first->second) << "\n";
    std::cout << "VAL END   " << (k.second->second) << "\n";
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
