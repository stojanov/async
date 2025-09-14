#pragma once

#include <async/defines.h>
#include <async/pch.h>

namespace async::internal::utils {
// struct observer_signal;
struct signal_connection {
    signal_connection() = default;

    signal_connection(std::function<void()> &&f) : _close_func(std::move(f)) {}

    void disconnect() { _close_func(); }

    signal_connection &operator=(const signal_connection &conn) {
        if (this != &conn) {
            _close_func = std::move(conn._close_func);
        }
        return *this;
    }

  private:
    std::function<void()> _close_func;
};

struct scoped_signal_connection : public signal_connection {
    scoped_signal_connection() : signal_connection() {}

    scoped_signal_connection(signal_connection &conn)
        : signal_connection(conn) {}

    scoped_signal_connection(signal_connection &&conn)
        : signal_connection(std::move(conn)) {}

    scoped_signal_connection &operator=(const signal_connection &conn) {
        signal_connection::operator=(conn);
        return *this;
    }

    ~scoped_signal_connection() {
        spdlog::error("CLOSED FROM SCOPED");
        disconnect();
    }
};

} // namespace async::internal::utils
