#pragma once

#include <async/io/pal/io_op.h>

namespace async::io::lin {
class io_op : public pal::io_op {
  public:
    io_op(pal::io_handle &handle) : pal::io_op(handle) {
        _fd = std::any_cast<int>(_handle.native());
    }

  protected:
    int _fd;
};
} // namespace async::io::lin
