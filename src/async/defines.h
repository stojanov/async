#pragma once

#include <chrono>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <memory>

namespace async {

using task_func = std::function<void()>;

template <typename T> using closure_task_func = std::function<void(T &closure)>;

using void_func = std::function<void()>;
using any_func = std::function<void(std::any)>;

using predicate_func = std::function<bool()>;
using cid_t = std::size_t;
using clk_t = std::chrono::high_resolution_clock;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

static constexpr u16 MAX_TIMERS = 16000;
static constexpr u16 MAX_THREADS = 16000;

static inline auto empty_void_func = []() {};

template <typename T> using s_ptr = std::shared_ptr<T>;
template <typename T> using u_ptr = std::unique_ptr<T>;

using duration_t = std::chrono::milliseconds;

using coro_handle = std::coroutine_handle<>;
} // namespace async
