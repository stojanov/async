#pragma once

#include <atomic>
#include <concepts>
#include <queue>

#include <async/defines.h>

namespace async {

template <std::integral T> class id_gen {
  public:
    [[nodiscard]] inline T get() {
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

template <std::integral T> class id_cont {};

static inline cid_t combine_u16(u16 a, u16 b) {
    return (a << (sizeof(u16) * 8)) | b;
}

static inline std::pair<u16, u16> unpack_u16(cid_t id) {
    const u16 n = 0xffff;

    const u16 b = id & n;
    const u16 a = id >> 16 & n;

    return std::pair(a, b);
}

static inline cid_t combine_u32(u32 a, u32 b) {
    const auto ua = (u64)a;
    const auto ub = (u64)b;
    return (ua << 32) | ub;
}

constexpr static inline std::pair<u32, u32> unpack_u32(cid_t id) {
    const u32 n = 0xffffffff;

    const u32 b = id & n;
    const u32 a = id >> 32 & n;

    return std::pair(a, b);
}

template <class... Ts> struct var_overload : Ts... {
    using Ts::operator()...;
};

template <typename T, typename Variant> struct is_in_variant;

template <typename T, typename... Ts>
struct is_in_variant<T, std::variant<Ts...>>
    : std::disjunction<std::is_same<T, Ts>...> {};

template <typename T, typename Variant>
concept is_in_variant_v = is_in_variant<T, Variant>::value;

template <typename R, typename V>
concept range_of =
    std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, V>;

} // namespace async
