#pragma once

#include <async/pch.h>

#include <iostream>
#include <iterator>
#include <spdlog/spdlog.h>

namespace async {

namespace detail {};

// V value should have equals operator, stongly encoraged for V to be a pointer
// untill i can fix it
template <std::integral K, typename V> class interval_map {
  public:
    using interval_t = std::pair<K, K>;
    using value_ref = V &;

  private:
    enum class interval_type { START, START_FILLER };

    // optimize multiple copies of this
    struct interval_entry {
        V value;
        K other_key_value;
        interval_type type;

        interval_entry *head_start;
        interval_entry *next_filler;

        // can the fillers be in another map?
        interval_entry create_filler_from_this() {
            return interval_entry{value, other_key_value,
                                  interval_type::START_FILLER};
        }

        static interval_entry create_filler(const interval_entry &entry) {
            return interval_entry{entry.value, entry.other_key_value,
                                  interval_type::START_FILLER};
        }
    };

    using mp_t = std::map<K, interval_entry, std::greater<K>>;
    using it_t = mp_t::iterator;

  public:
    void add_interval(const interval_t &interval, const V &value) {
        add_interval_impl(interval, value);
    }

    // consumes the range/interval
    void add_interval_overlap_filler_clear(const interval_t &interval,
                                           const V &value) {
        const auto &[start, end] = interval;
        auto [i, my_range_end] = add_interval_impl(interval, value);

        auto const end_start = std::end(_mp_start);

        std::function<bool(it_t)> predicate = [end](it_t i) {
            return i->first < end &&
                   i->second.type == interval_type::START_FILLER;
        };

        if (my_range_end != end_start) {
            predicate = [](it_t i) {
                return i->second.type == interval_type::START_FILLER;
            };
        }

        std::advance(i, -1);
        remove_if(i, my_range_end, predicate, -1);
    }

    void update_interval(const interval_t &original,
                         const interval_t &new_interval) {}

    std::optional<V *> find_in_interval(const K &interval_value) {
        const auto lower = _mp_start.lower_bound(interval_value);

        if (lower == std::end(_mp_start) ||
            lower->second.other_key_value < interval_value) {
            return std::nullopt;
        }

        return &lower->second.value;
    }

    void print() {
        std::cout << "START POINT MAP\n";

        auto type_str = [](interval_type t) {
            return t == interval_type::START_FILLER ? "FILLER" : "START";
        };

        for (auto &i : _mp_start) {
            spdlog::info("{}:{} ({})", i.first, i.second.value,
                         type_str(i.second.type));
        }
    }

  private:
    std::pair<it_t, it_t> add_interval_impl(const interval_t &interval,
                                            const V &value,
                                            bool consume = false) {
        const auto &[start, end] = interval;

        const auto end_start = std::end(_mp_start);

        auto start_entry = interval_entry{value, end, interval_type::START};

        // check if filler, how does that factor
        auto lower_node_start = _mp_start.lower_bound(start);

        // TODO: think about
        it_t my_range_end = end_start;

        if (lower_node_start != end_start && lower_node_start->first > start) {
            const auto prev_start = lower_node_start->first;
            const auto prev_end = lower_node_start->second.other_key_value;

            if (prev_start > end) {
                // do nothing
            } else {
                if (prev_end > end) {
                    _mp_start.erase(lower_node_start);

                    // TODO: think about if _mp_start[end] already has a point
                    const auto [i, _] =
                        _mp_start.insert({end, interval_entry::create_filler(
                                                   lower_node_start->second)});

                    my_range_end = i;
                } else {
                }
            }
        } else if (lower_node_start != end_start &&
                   lower_node_start->first < start) {
            auto &parent_end = lower_node_start->second.other_key_value;

            if (end > parent_end) {
                parent_end = start;
            } else {
                const auto [i, _] = _mp_start.insert(
                    {end,
                     interval_entry::create_filler(lower_node_start->second)});

                my_range_end = i;
            }
        }

        auto higher_node_start = _mp_start.lower_bound(end);

        if (higher_node_start != std::end(_mp_start)) {
            auto prev_end = higher_node_start->second.other_key_value;

            if (prev_end < end) {
                // What if other is here ?
                _mp_start.insert(
                    {prev_end, interval_entry::create_filler(start_entry)});
            }
        } else {
            // nothing
        }

        auto [i, _] = _mp_start.insert({start, start_entry});

        return {i, my_range_end};
    }

    // [start, end)
    void remove_if(it_t start, it_t end, std::function<bool(it_t)> pred,
                   int incr) {

        const auto mp_end = std::end(_mp_start);
        const auto mp_begin = std::begin(_mp_start);

        auto i = start;
        while (true) {
            if (i == end || i == mp_end || i == mp_begin) {
                break;
            }

            if (pred(i)) {
                auto i_copy = i;
                std::advance(i_copy, incr);

                _mp_start.erase(i);

                i = i_copy;
                continue;
            }

            std::advance(i, incr);
        }
    }

  private:
    mp_t _mp_start;
};
} // namespace async
