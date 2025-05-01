#pragma once

#include <async/pch.h>

namespace async {

template <bool is_synced = true> class sync_op {
  public:
    // Cannot have arguments in the function
    // find a way to signal this at compile time, with a pretty print
    // maybe a performance bottleneck when capture the whole reference object,
    // this not sure what kind of optimization can the compiler do here if it's
    // not a raw function pointer
    template <std::invocable F, typename... Args>
    auto do_op(const F &f, Args... args) {
        if constexpr (is_synced) {
            std::lock_guard lck(_M);
            return f(std::forward<Args>(args)...);
        } else {
            return f(std::forward<Args>(args)...);
        }
    };

    // assumes you don't do somethign bad with the pointer, as to delete it
    std::mutex &underlying_mutex() { return _M; }

  private:
    std::mutex _M;
};

} // namespace async
