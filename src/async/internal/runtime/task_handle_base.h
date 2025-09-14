#pragma once

#include <async/internal/defines.h>

namespace async::internal {

struct runtime;

// has to be defined here since
// task_handle<T> <-> runtime need to talk to each other
// i.e recursive includes
struct task_handle_base {
    task_handle_base(cid_t id) : _id(id) {}

    virtual ~task_handle_base() = default;

    friend runtime;

  private:
    virtual void on_result(void *o) = 0;

  public:
    // not needed possilby
    cid_t _id;
};

} // namespace async::internal
