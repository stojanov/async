#pragma once

#include <async/internal/io/pal/io_handle.h>
#include <async/internal/io/pal/io_type.h>

namespace async::io::pal {

class io_op {
  public:
    io_op(io_handle &h) : _handle(h) {}

    virtual void do_op() = 0;
    virtual io_type type() const = 0;

    io_handle &handle() { return _handle; }

  protected:
    io_handle &_handle;
};
} // namespace async::io::pal
