#pragma once

#include <async/internal/cont/midpoint_map.h>
#include <async/internal/cont/sparse_set.h>
#include <async/internal/pch.h>
#include <cwchar>

namespace async::internal {

template <typename T, typename H_f = std::hash<T>> class sparse_set_list {
    static constexpr auto hasher = H_f{};

  public:
    class iterator {
      private:
        T *ptr;

      public:
        explicit iterator(T *p) : ptr(p) {}

        T &operator*() { return *ptr; }

        iterator &operator++() {
            ++ptr;
            return *this;
        }

        iterator operator++(iterator) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator &other) const {
            return ptr == other.ptr;
        }

        bool operator!=(const iterator &other) const {
            return ptr != other.ptr;
        }
    };

  public:
    // TODO: figure out optimal block size
    sparse_set_list(std::size_t block_size) {}

    void add(const T &value) {
        auto id = hasher(value);

        auto [range_start, range_end] = _mp.get_all_prio();

        // if no available, add new
        // _mp always grows linearly intervally
        // at some point indexes will become cluttered
    }

    void add(T &&value) {}

    void find_hash() {}

    void remove();

  private:
    using set_ptr = u_ptr<associative_sparse_set<T, H_f>>;

    midpoint_map<set_ptr> _mp;
    // TODO: bridge in one ^
    //
    // lot of cid_t(unique, block size N) map to a single u32
    // what will sort of key do?
    // TODO, integrate interval map into it
    std::unordered_map<cid_t, u32> _key_relation;
};

} // namespace async::internal
