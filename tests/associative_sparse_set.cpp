#include <async/internal/cont/sparse_set.h>
#include <gtest/gtest.h>

struct TestValue {
    int a{0};
    int id{0};
};

template <> struct std::hash<TestValue> {
    std::size_t operator()(const TestValue &s) const noexcept { return s.id; }
};

using namespace testing;
class SparseSetTests : public testing::Test {
    void SetUp() override {
        static constexpr auto SIZE = 1000;
        // async::associative_sparse_set<dd, typename V>

        // _set = new async::associative_sparse_set<TestValue,
        // std::size_t>(SIZE);
        std::cout << "SETUP IS CALLED\n";
    }

    // simple clear would suffice
    void TearDown() override { delete _set; }

  protected:
    void fill_dummy_data() {
        std::cout << "ADDING FROM OUTSIDE\n";
        // _set->add({4, 0});
        // _set->add({3, 1});
        // _set->add({2, 2});
        // _set->add({1, 3});
        // _set->add({0, 4});
    }

    // void print() {
    //     for (auto k : *_set) {
    //         std::cout << "ID: " << k.id << ", a: " << k.a << "\n";
    //     }
    // }

    async::internal::associative_sparse_set<TestValue, std::size_t> *_set;
};

TEST_F(SparseSetTests, EnsureITISDelet) {
    fill_dummy_data();

    // _set->remove_hash(2);

    // print();
}
