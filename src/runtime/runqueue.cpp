#include <runtime/defines.h>
#include <runtime/runqueue.h>

namespace async::runtime {
runqueue::runqueue() {}

void runqueue::push_pending(task_block &block) {
    std::cout << "GOT \n";
    {
        std::unique_lock lck(_qM);
        _active.push_back(block);
    }

    _q_signal.notify_one();
}
void runqueue::push_pending(task_block &&block) {
    {
        std::unique_lock lck(_qM);
        _active.push_back(std::move(block));
    }

    _q_signal.notify_one();
}

// CONSIDER EXIT CASE, SHUTDOWN
task_block runqueue::peek_pending() {
    std::unique_lock lck(_qM);
    _q_signal.wait(lck, [this]() { return !_active.empty(); });

    const task_block block = _active.front();

    return block;
}

// add try_pop
void runqueue::pop_pending() {
    std::unique_lock lck(_qM);
    _q_signal.wait(lck, [this]() { return !_active.empty(); });

    _active.pop_front();
}

task_block runqueue::peek_pop_pending() {
    std::unique_lock lck(_qM);
    _q_signal.wait(lck, [this]() { return !_active.empty(); });

    const task_block block = _active.front();
    _active.pop_front();

    return block;
}

} // namespace async::runtime
