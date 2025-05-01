#pragma once

#include <async/defines.h>
#include <async/io/pal/io_op.h>
#include <type_traits>

namespace async::io::pal {

template <typename T> class read_op : public T {
    static_assert(std::is_base_of_v<io_op, T>);

  public:
    read_op(io_handle &handle) : T(handle) {};

    virtual std::size_t read(bytespan span, std::size_t size = 0) = 0;
};

} // namespace async::io::pal
