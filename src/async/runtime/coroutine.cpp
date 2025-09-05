#include <async/runtime/coroutine.h>
#include <async/runtime/runtime.h>
#include <async/runtime/runtime_core.h>

namespace async {

std::suspend_never promise::final_suspend() noexcept {

    spdlog::warn("FINAL SUSPEND");

    internal::runtime::inst().remove_coro(_id);

    return {};
}

} // namespace async
