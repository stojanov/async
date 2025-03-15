#include "async/utils.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>

#include <async/defines.h>
#include <async/pch.h>

namespace async {

namespace detail {

u64 create_key(u32 key, u32 ready) {
    if (ready) {
        return combine_u32(key, ready);
    }
    return combine_u32(key, ready);
}

struct sorter_less {
    constexpr bool operator()(const u64 &lhs, const u64 &rhs) const {
        const auto [rk, rr] = unpack_u32(rhs);

        // TODO: remove branching
        if (lhs == 0 && rr > 0) {
            return false;
        }

        const auto [lk, lr] = unpack_u32(lhs);
        if (rhs == 0 && lr > 0) {
            return true;
        }

        /*if (lr == rr && rr > 0) {*/
        /*    return true;*/
        /*}*/

        u64 left = lk;
        u64 right = rk;

        const auto blr = static_cast<int>((bool)lr);
        const auto brr = static_cast<int>((bool)rr);

        const auto afl = (u32_m * blr) + (blr * left);
        const auto afr = (u32_m * brr) + (brr * right);

        const auto left_factor = afl + (blr * lr);
        const auto right_factor = afr + (brr * rr);

        left = blr * left_factor + ((!blr) * left);
        right = brr * right_factor + ((!brr) * right);

        // make this more elegant, merge it into the main numbers, branchless
        if (left == right && lk > rk) {
            return false;
        }

        if (left == right && rk > lk) {
            return true;
        }

        // cover equals here maybe manually
        return left < right;
    }
};

} // namespace detail

// TODO: extend, generic
template <typename ValueT> class midpoint_map {
  private:
    using mp_t = std::map<u64, ValueT, detail::sorter_less>;
    using it_t = mp_t::iterator;

  public:
    // TODO: extend it
    class value_key_it : public it_t {
      public:
        std::pair<u32, ValueT *> operator*() const {
            auto &n = it_t::operator*();

            const auto [key, _] = unpack_u32(n.first);

            return std::pair(key, &n.second);
        }
    };

    midpoint_map() { mp[0] = ValueT(); }

    void add(u32 key, const std::string &m, u32 factor) {
        mp[detail::create_key(key, factor)] = m;
    }

    void toggle_add(u32 key, bool ready) {}
    void toggle(u32 key, bool ready) {}

    it_t find(u32 id) {
        auto midpoint = mp.find(0);

        // TODO: write tests
        // ensure checks on ranges at the benigning
        //
        const auto l = mp.size();

        if (l == 0) {
            return mp.end();
        }

        if (l == 1) {
            return mp.begin();
        }

        if (l == 2) {
            for (auto i = mp.begin(); i != mp.end(); i++) {
                if (i->first != 0) {
                    return i;
                }
            }
        }

        auto end = mp.end();
        std::advance(end, -1);

        const auto [highest_normal_index, _] = unpack_u32(end->first);

        auto lower = midpoint;
        auto higher = midpoint;

        std::advance(lower, -1);
        std::advance(higher, 1);

        if (higher != mp.end()) {
            auto [_id, ready] = unpack_u32(higher->first);

            if (_id == id) {
                return higher;
            }

            if (id > _id && id <= highest_normal_index) {
                auto new_key = combine_u32(id, 0);

                auto value = mp.find(new_key);

                if (value != mp.end()) {
                    return value;
                } else {
                    std::cout << "NOT POSSIBLE, DIDN't FIND IT \n";
                }
            }
        }

        const auto new_key = combine_u32(id, 1);
        auto value = mp.lower_bound(new_key);

        const auto unpacked = unpack_u32(value->first);

        if (unpacked.first == id) {
            return value;
        }

        // TODO: think about edge cases, return end when no key is in the ready
        // domain
        std::advance(value, 1);

        return value;
    }

    // In essense return only the given prio, multiple values if same prio
    // custom end iterator, midpoint or begin depends on order
    // custom iterator range, how to query fast the whole range
    // add option to see if there are ready by priority fast, check out bloom
    // filter
    //
    // return a range
    std::pair<it_t, it_t> find_prio(u32 prio) {
        // can i get the distance/size of things with same prio can be min max,
        // in possibly logn time without N
        auto new_key = combine_u32(0, prio);
        auto value = mp.upper_bound(new_key);

        const auto unpacked = unpack_u32(value->first);

        std::cout << "START OF VALUE " << unpacked.first << ", "
                  << unpacked.second << "\n";

        if (value == mp.end()) {
            std::cout << "VALUE END\n";
            return {value, value};
        }

        /*if (unpacked.second == prio) {*/
        /*    return value;*/
        /*}*/

        // TODO: think about edge cases, return end when no key is in the ready
        // domain
        if (unpacked.second != prio) {
            std::cout << "ADVANCED \n";
            /*std::advance(value, 1);*/
        }

        auto upper_bound = combine_u32(0, prio + 1);
        // can this be omptimized it will still be logn but with a custom tree
        // i can define the start node to search in theory O()
        auto value_upper = mp.lower_bound(upper_bound);

        if (value_upper == mp.end()) {
            std::cout << "END\n";
            return {value, mp.end()};
        }

        auto upper_unpacked1 = unpack_u32(value_upper->first);

        std::cout << "SECOND START " << value_upper->second << "\n";

        std::advance(value_upper, -1);

        auto upper_unpacked = unpack_u32(value_upper->first);

        std::cout << "SECOND " << value_upper->second << "\n";

        if (upper_unpacked.second != prio) {
            return {value, mp.end()};
        }

        /*if (value_upper == value) {*/
        /*    return { value;*/
        /*}*/
        /**/
        /*// sanity check if same prio if not same it*/
        /*if (upper_unpacked.second != prio) {*/
        /*}*/

        // domain
        return {value, value_upper};
    }

    void find_prio_range(u32 min, u32 max) {}

    void print(std::string_view name) {
        std::cout << "MIDPOINT MAP " << name << "\n";
        for (auto &i : mp) {
            const auto [id, factor] = unpack_u32(i.first);
            std::cout << "\tID:" << id << "\tVAL: " << i.second
                      << "\tRID:" << i.first << "\tRF:" << factor << "\n";
        }
    }

    // override iterator to give the correct key
    value_key_it value_key_begin() { return value_key_it(mp.begin()); }
    value_key_it value_key_end() { return value_key_it(mp.end()); }

  private:
    mp_t mp;
};

} // namespace async
