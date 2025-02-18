#pragma once

#include <atomic>
#include <concepts>
#include <queue>
#include <type_traits>

#include <defines.h>

#include <iostream>

namespace async {

template <std::integral T> class id_gen {
  public:
    [[nodiscard]] T get() {
        {
            std::lock_guard lck(_available_m);
            if (!_available.empty()) {
                T rtn = _available.back();
                _available.pop();
                return rtn;
            }
        }

        return _counter.fetch_add(1);
    }

    void drop(T id) {
        std::lock_guard lck(_available_m);
        _available.push(id);
    }

    void reset() {
        std::lock_guard lck(_available_m);
        std::queue<T> empty;
        _available.swap(empty);

        _counter = 0;
    }

  private:
    // TODO: profile this, lockless queue, best way probably
    std::mutex _available_m;
    std::queue<T> _available;
    std::atomic<T> _counter{0};
};

static inline cid_t combine_u16(u16 a, u16 b) {
    return (a << (sizeof(u16) * 8)) | b;
}

static inline std::pair<u16, u16> unpack_u16(cid_t id) {
    const u16 n = 0xffff;

    const u16 b = id & n;
    const u16 a = id >> 16 & n;

    return std::pair(a, b);
}

template <class... Ts> struct var_overload : Ts... {
    using Ts::operator()...;
};

template <typename R, typename V>
concept range_of =
    std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, V>;
;
} // namespace async
