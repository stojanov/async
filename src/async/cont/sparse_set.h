#pragma once

#include <async/pch.h>

namespace async {

template <typename T, typename A> class sparse_set {
    using size_t = std::vector<T>::size_type;

  public:
    std::optional<size_t> add(const T &value, const A &association) {}
    std::optional<size_t> add(T &&value, const A &association) {}

    bool remove(size_t idx) {}
    bool remove(const A &value) {}

    bool has_available() {}

    std::optional<T> find(size_t idx) {}

  private:
    std::vector<T> _data;

    size_t _max;
    size_t _current;
};

} // namespace async
