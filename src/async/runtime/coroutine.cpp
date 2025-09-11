#include <async/runtime/coroutine.h>

#include <async/runtime/runtime.h>
#include <async/runtime/runtime_core.h>

namespace async {

void promise_base::on_shutdown() {
    internal::runtime::inst().notify_result(_id, _value_ptr);
}

} // namespace async
