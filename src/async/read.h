#pragma once

#include <async/io/base.h>
#include <async/pch.h>
#include <async/runtime/runtime.h>
#include <memory>
#include <thread>

namespace async {

namespace internal {

// TODO: THINK ABOUT LIFETIME, OWNERSHIP
class read_core : public io::read_op {
  public:
    read_core(io::pal::io_handle &handle) : io::read_op{handle} {}

    void add_waiting(coro_handle h) { _waiting = h; }

    void do_op() override {
        auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        spdlog::warn("RESUMED {}", id);
        internal::runtime::inst().submit_resume(_waiting);
    }

  private:
    coro_handle _waiting;
};

class read_handle {};

struct read_awaitable_t {
    read_awaitable_t(io::pal::io_handle &handle, bytespan span,
                     std::size_t nbytes)
        : _span(span), _nbytes(nbytes) {
        _core = std::make_shared<read_core>(handle);
    }

    bool await_ready() { return false; }

    void await_suspend(coro_handle h) {
        auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        spdlog::warn("SUSPENDED {}", id);
        _core->add_waiting(h);
        internal::runtime::inst().submit_io_op(_core);
    }

    std::size_t await_resume() {
        auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        spdlog::warn("DOING READ FROM THREAD {}", id);
        return _core->read(_span, _nbytes);
    }

  private:
    s_ptr<read_core> _core;
    bytespan _span;
    std::size_t _nbytes;
};
} // namespace internal

inline internal::read_awaitable_t read(io::pal::io_handle &handle,
                                       bytespan span, std::size_t nbytes) {
    return {handle, span, nbytes};
}

} // namespace async
