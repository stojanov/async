#pragma once

#include <type_traits>
#include <typeinfo>
namespace async::internal {

// main use would be to pass object within the same scope
// queues, channels, signals, value passing between entities mainly
// with the caveat that it must be in the same calling scope
class any_ptr {
  private:
    struct type_base {
        virtual const std::type_info &type_info() = 0;
    };

    template <typename T> struct type_data : public type_base {
        const std::type_info &type_info() final { return typeid(T); }
    };

  public:
    template <typename T>
    any_ptr(T *o)
        : _o(static_cast<void *>(const_cast<std::remove_cv_t<T> *>(o))),
          _type_base(std::make_unique<type_data<T>>()) {}

    const std::type_info &type_info() const { return _type_base->type_info(); }
    void *data() const { return _o; }

  private:
    void *_o{nullptr};
    std::unique_ptr<type_base> _type_base;
};

} // namespace async::internal
