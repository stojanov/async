#pragma once

#include <async/internal/io/pal/io_op.h>
#include <async/internal/pch.h>

namespace async::io::pal {

template <typename T> class read_op : public T {
    static_assert(std::derived_from<T, io_op>);

  public:
    read_op(io_handle &handle) : T(handle) {};

    virtual std::size_t read(bytespan span, std::size_t size = 0) = 0;
};

} // namespace async::io::pal
