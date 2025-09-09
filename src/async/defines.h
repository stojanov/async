#pragma once

#include <async/runtime/coroutine.h>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace async {

using cid_t = std::size_t;
using clk_t = std::chrono::high_resolution_clock;
using duration_t = clk_t::duration;
using bytespan = std::span<std::byte>;

using void_func = std::function<void()>;
using any_func = std::function<void(std::any)>;

template <typename T>
using coroutine_result_func = std::function<coroutine<T>()>;

using coroutine_void_func = std::function<coroutine<>()>;
using coroutine_any_func = std::function<coroutine<>(std::any &)>;
using predicate_func = std::function<bool()>;
} // namespace async

namespace async::internal {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

static inline auto empty_void_func = []() {};

template <typename T> using s_ptr = std::shared_ptr<T>;
template <typename T> using u_ptr = std::unique_ptr<T>;

using coro_handle = std::coroutine_handle<>;

enum class value_state { READY = 0, CONSUMED, NOTIFY };

using value_state_func =
    std::function<bool(value_state, cid_t id, std::optional<std::any>)>;

const auto u32_m = std::numeric_limits<std::uint32_t>::max();
const auto u64_m = std::numeric_limits<std::uint64_t>::max();
} // namespace async::internal
