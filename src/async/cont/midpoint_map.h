#pragma once

#include "async/utils.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>

#include <async/defines.h>
#include <async/pch.h>
#include <optional>

namespace async::internal {

static inline u64 midpoint_create_key(u32 key, u32 ready) {
    if (ready) {
        return combine_u32(key, ready);
    }
    return combine_u32(key, ready);
}

struct midpoint_sorter_less {
    // this needs to be as fast a possible
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

        const auto afl = (u32_m * blr);
        const auto afr = (u32_m * brr);

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

// TODO: extend, generic ?
template <typename ValueT> class midpoint_map {
  private:
    // using a formula you can directly query the ranges metadata, by using the
    // interval you get a range "prefix" sum in about 2 log n, while also
    // getting some extra functionality built in, use variant to specify
    // metadata for a range, sort function should account for this "offset" in a
    // way, this would be the one that provides the range addition, if every
    // ready factor unique double space complexity, also maximum range of ready
    // factor is smaller
    using mp_t = std::map<u64, ValueT, midpoint_sorter_less>;
    using it_t = mp_t::iterator;

  public:
    class skip_mid_it : public it_t {
      public:
        // needs decrement as well
        it_t &operator++() noexcept {
            it_t::operator++();

            if (it_t::operator*().first == 0) {
                it_t::operator++();
            }

            return *this;
        }

        it_t operator++(int) noexcept {
            it_t temp = *this;
            it_t::operator++(1);

            if (it_t::operator*().first == 0) {
                it_t::operator++(1);
            }

            return temp;
        }

        it_t &operator--() noexcept {
            it_t::operator--();

            if (it_t::operator*().first == 0) {
                it_t::operator--();
            }

            return *this;
        }

        it_t operator--(int) noexcept {
            it_t temp = *this;
            it_t::operator--(1);

            if (it_t::operator*().first == 0) {
                it_t::operator--(1);
            }

            return temp;
        }
    };

    // TODO: extend it
    class value_key_it : public skip_mid_it {
      public:
        using pair_t = std::pair<u32, ValueT *>;

        value_key_it(it_t it) : skip_mid_it(it) {}

        pair_t operator->() const { return value_key_it::operator*(); }

        pair_t operator*() const {
            auto &n = skip_mid_it::operator*();

            const auto [key, _] = unpack_u32(n.first);

            return std::pair(key, &n.second);
        }

        skip_mid_it raw_it() { return this; }
    };

    midpoint_map() { mp[0] = ValueT(); }

    it_t add(u32 key, const ValueT &m, u32 factor) {
        auto iter = mp.insert(std::pair(midpoint_create_key(key, factor), m));

        return iter.first;
    }

    it_t add(u32 key, const ValueT &&m, u32 factor) {
        auto iter = mp.insert(
            std::pair(midpoint_create_key(key, factor), std::move(m)));

        return iter.first;
    }

    void remove(u32 key) {
        spdlog::warn("MAP ERASE {}", key);
        mp.erase(find(key));
    }

    void remove(it_t i) { mp.erase(i); }

    bool modify_prio_by_factor(u32 key, int factor) {
        auto found = find(key);
        auto [_, old_prio] = unpack_u32(found->first);

        u32 new_prio = old_prio;
        if (factor < 0 && old_prio <= factor) {
            new_prio = 0;
        } else if (factor > 0 && ((old_prio * factor) > u32_m)) {
            new_prio = u32_m;
        } else {
            new_prio = old_prio + factor;
        }

        return update_prio(found, new_prio);
    }

    bool update_prio(u32 key, u32 prio) {
        auto found = find(key);
        return update_prio(found, prio);
    }

    bool update_prio(u32 key, std::function<u32(const ValueT &)> adapter) {
        auto found = find(key);
        if (found == std::end(mp)) {
            return false;
        }
        return update_prio(found, adapter(found->second));
    }

    bool update_prio(it_t i, u32 prio) {
        if (i == mp.end()) {
            return false;
        }

        auto [id, _] = unpack_u32(i->first);

        auto value = std::move(i->second);
        mp.erase(i);
        mp.insert(std::pair(midpoint_create_key(id, prio), std::move(value)));

        // spdlog::warn("MAP UPDATE {} SIZE {}", id, mp.size());

        return true;
    }

    it_t find(u32 id) {
        auto midpoint = mp.find(0);

        // TODO: write tests
        // ensure checks on ranges at the benigning
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

        auto lower = midpoint;
        auto higher = midpoint;

        std::advance(lower, -1);
        std::advance(higher, 1);

        if (higher != mp.end()) {
            auto end = mp.end();
            std::advance(end, -1);

            const auto [highest_normal_index, _] = unpack_u32(end->first);
            auto [_id, prio_factor] = unpack_u32(higher->first);

            if (_id == id) {
                return higher;
            }

            if (id > _id && id <= highest_normal_index) {
                auto new_key = combine_u32(id, 0);

                auto value = mp.find(new_key);

                if (value != mp.end()) {
                    return value;
                } else {
                    std::cout << "NOT POSSIBLE, DIDN'T FIND IT \n";
                }
            }
        }

        const auto new_key = combine_u32(id, 1);
        auto value = mp.lower_bound(new_key);

        // check if end it
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
    std::pair<it_t, std::optional<it_t>> find_prio_range(u32 min_prio,
                                                         u32 max_prio) {
        // can i get the distance/size of things with same prio can be min max,
        // in possibly logn time without N
        //
        if (min_prio == 0) {
            // when extended the right side to something
            // then we do extra shit here!
            auto midpoint = mp.find(0);
            std::advance(midpoint, 1);
            return {midpoint, mp.end()};
        }

        auto new_key = combine_u32(0, min_prio);
        auto value = mp.upper_bound(new_key);

        const auto unpacked_start_node = unpack_u32(value->first);

        if (value == mp.end()) {
            return {value, std::nullopt};
        }

        if (unpacked_start_node.second != min_prio) {
            return {mp.end(), std::nullopt};
        }

        const auto upper_bound_key = combine_u32(0, max_prio);
        // can this be omptimized it will still be logn but with a custom tree
        // i can define the start node to search in theory worst O(log n), best
        // smaller not really that much speed up but still a point to think
        // about
        auto upper_node = mp.lower_bound(upper_bound_key);

        // TODO: cannot assume that we return end here, what if there are no
        // stuff here!, what we need to check if we are still at the same
        if (upper_node == mp.end()) {
            std::advance(upper_node, -1);

            if (upper_node == mp.end()) {
                // impossible
                return {value, std::nullopt};
            }

            const auto [_, upper_prio] = unpack_u32(upper_node->first);

            if (upper_prio == min_prio) {
                return {value, mp.end()};
            }

            // Rare
            return {value, std::nullopt};
        }

        auto [_, upper_prio] = unpack_u32(upper_node->first);
        if (upper_prio == 0) {
            return {value, std::nullopt};
        }

        return {value, upper_node};
    }

    std::pair<it_t, std::optional<it_t>> find_prio(u32 min) {
        return find_prio_range(min, min + 1);
    }

    std::pair<it_t, it_t> get_all_prio() { return {mp.begin(), mp.find(0)}; }
    std::pair<it_t, it_t> get_all_latter() {
        auto it = mp.lower_bound(midpoint_create_key(1, 0));

        auto [id_low, prio_low] = unpack_u32(it->first);
        if (id_low == 0 && prio_low == 0) {
            it = mp.end();
        } else if (prio_low != 0) {
            it = mp.end();
        }

        return {it, mp.end()};
    }

    void print(std::string_view name) {
        std::cout << "MIDPOINT MAP " << name << "\n";
        for (auto &i : mp) {
            const auto [id, factor] = unpack_u32(i.first);
            std::cout << "\tID:" << id << "\tVAL: "
                      << "\tRID:" << i.first << "\tRF:" << factor << "\n";
        }
    }

    std::size_t size() { return mp.size(); }

    bool empty() { return mp.size() == 0; }

    // TODO: has to skip the midpoint
    // override iterator to give the correct key
    value_key_it value_key_begin() { return value_key_it(mp.begin()); }
    value_key_it value_key_end() { return value_key_it(mp.end()); }

    // these should be const
    skip_mid_it begin() { return skip_mid_it(mp.begin()); }
    skip_mid_it end() { return skip_mid_it(mp.end()); }

    it_t raw_begin() { return mp.begin(); }
    it_t raw_end() { return mp.end(); }

    it_t midpoint_it() { return mp.find(0); }

  private:
    mp_t mp;
};

} // namespace async::internal
