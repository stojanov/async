#include <gtest/gtest.h>

#include <async/internal/defines.h>

/*using namespace async;*/
/**/
/*struct Key {*/
/*    u64 id;*/
/*    u64 factor;*/
/*};*/
/**/
/*template <typename K, typename V> struct KeyHasher {*/
/*    // ours*/
/*    std::size_t operator()(const Key &k) { return 0; }*/
/**/
/*  protected:*/
/*    V *_value;*/
/*};*/
/**/
/*template <typename K, typename V> struct GenericMapHasher {*/
/*    virtual u64 sort_factor(const K &key, const V &value) {}*/
/*    virtual u64 id(const K &key, const V &value) {}*/
/**/
/*  protected:*/
/*    GenericMapHasher(V *value) {}*/
/*};*/
/**/
/*class GenericMidpointTests : public testing::Test {*/
/*    using mp_t = async::midpoint_generic<Key, std::string>;*/
/**/
/*    void SetUp() override { mp = std::make_shared<mp_t>(); }*/
/**/
/*    void TearDown() override { mp.reset(); }*/
/**/
/*  protected:*/
/*    std::shared_ptr<mp_t> mp;*/
/*};*/
