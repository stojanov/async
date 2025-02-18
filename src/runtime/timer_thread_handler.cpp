#include "runtime/timer_thread_handler.h"
#include "utils.h"

namespace async::runtime {
void timer_thread_handler::remove_timer(cid_t id) {
    auto [th_block_id, timer_id] = unpack_u16(id);

    if (auto timer_block = find_timer(th_block_id)) {
        auto &th = std::get<timer_thread>(*timer_block->thread_work);

        th.remove_timer(timer_id);
    }
}
} // namespace async::runtime
