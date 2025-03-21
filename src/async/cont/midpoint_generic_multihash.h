#pragma once

namespace async {

namespace detail {

template <typename K, typename V> struct wrapped_key {
    K key;
    V *value;
};

template <typename K, typename H> struct mid_generic_sorter_less {
    static constexpr auto hasher = H{};

    constexpr bool operator()(const K &lhs, const K &rhs) const {
        return hasher(lhs) < hasher(rhs);
    }
};

} // namespace detail

template <typename K, typename V> struct KeyMapper {
    virtual u64 sort_factor(const K &key, const V &value) = 0;
    virtual u64 id(const K &key, const V &value) = 0;
};

template <typename K, typename V, typename H = std::hash<K>,
          typename S = detail::mid_generic_sorter_less<K, H>>
class midpoint_generic {
  public:
  private:
};

} // namespace async
