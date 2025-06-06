#pragma once

#include <async/pch.h>
#include <async/io/pal/io_op.h>

namespace async::io::pal {

template <typename T> class read_op : public T {
    static_assert(std::derived_from<T, io_op>);

  public:
    read_op(io_handle &handle) : T(handle) {};

    virtual std::size_t read(bytespan span, std::size_t size = 0) = 0;
};

} // namespace async::io::pal
