#pragma once

namespace async::io::pal {

class io_handle {
  public:
    virtual std::any native() const = 0;

    virtual std::size_t hash_code() const = 0;
};
} // namespace async::io::pal
