#pragma once

#include <async/pch.h>

namespace async::runtime {
class runtime_core;

class worker_thread {
  public:
    worker_thread(runtime_core &core);

    void signal_shutdown();
    void work();

  private:
    s_ptr<std::atomic_bool> _running;
    runtime_core &_core;
};
} // namespace async::runtime
