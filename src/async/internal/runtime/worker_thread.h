#pragma once

#include <async/internal/pch.h>

namespace async::internal {
class runtime_core;

class worker_thread {
  public:
    worker_thread(runtime_core &core);

    void activate_pending_work();

    runtime_core &_core;
};
} // namespace async::internal
