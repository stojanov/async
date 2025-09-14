#pragma once

#include <async/internal/io/pal/op/read.h>
#include <async/internal/io/platform/linux/io_op.h>
#include <unistd.h>

namespace async::io::lin {
class read_op : public pal::read_op<io_op> {
  public:
    read_op(pal::io_handle &h) : pal::read_op<io_op>(h) {}

    std::size_t read(bytespan span, std::size_t nbytes = 0) override {
        auto _nbytes = nbytes == 0 ? span.size() : nbytes;

        return ::read(_fd, span.data(), _nbytes);
    };

    pal::io_type type() const override { return pal::io_type::IN; }
};
} // namespace async::io::lin
