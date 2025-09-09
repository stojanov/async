#include <async/runtime/coroutine.h>

#include <async/runtime/runtime.h>
#include <async/runtime/runtime_core.h>

namespace async {

void promise_base::on_shutdown() {
    spdlog::warn("FINAL SUSPEND");
    internal::runtime::inst().remove_coro(_id);
}

} // namespace async
