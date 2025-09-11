#pragma once

#include <coroutine>
#include <spdlog/spdlog.h>
#include <typeindex>

namespace async {

template <typename T> struct promise;

template <typename T = void>
struct coroutine : std::coroutine_handle<promise<T>> {
    using promise_type = async::promise<T>;
    using value_type = T;
};

struct promise_base {
    void on_shutdown();

    std::size_t _id;
    void *_value_ptr{nullptr};
    // might be needed, might not
    // std::type_index _t_idx;
};

template <typename T> struct promise : public promise_base {
    coroutine<T> get_return_object() {
        return {coroutine<T>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept {
        on_shutdown();
        return {};
    }

    void return_value(T &&v) noexcept(std::is_nothrow_move_constructible_v<T>) {
        _value = std::move(v);
        _value_ptr = &_value;
    }

    void unhandled_exception() {}

    // has to be default constructable, sadly
    T _value;
};

template <> struct promise<void> : public promise_base {
    coroutine<void> get_return_object() {
        return {coroutine<void>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept {
        on_shutdown();
        return {};
    }

    void return_void() { spdlog::warn("RETURN VOID"); }

    void unhandled_exception() {}
};

} // namespace async
