#pragma once

#include <async/cont/midpoint_map.h>
#include <async/pch.h>
#include <concepts>

namespace async {

template <typename T, typename V>
concept SecondPrio = requires(T t, V v) {
    { t.prio(v) } -> std::convertible_to<u32>;
};

template <typename ValueT, SecondPrio<ValueT> second_prio>
class midpoint_map_adapter {
  private:
    using mp_t = midpoint_map<ValueT>;

  public:
    using it_t = decltype(midpoint_map<ValueT>().begin());
    static auto constexpr prio_adapter = second_prio{};

    void add(u32 key, const ValueT &v) { _map.add(key, v, prio_adapter(v)); }

    void remove(u32 key) { _map.remove(key); };

    void remove(it_t i) { _map.remove(i); }

    void update(u32 key) {
        _map.update_prio(key, [](const ValueT &v) { return prio_adapter(v); });
    };

    std::pair<it_t, it_t> get_all_prio() { return _map.get_all_prio(); }

  private:
    mp_t _map;
};

} // namespace async
