#pragma once

#include <async/defines.h>
#include <async/pch.h>
#include <async/runtime/runtime.h>
#include <async/runtime/task_handle_base.h>
#include <async/util/buffered_observer_signal.h>
#include <async/util/signal_connection.h>

// BIT OF SPAGETTI CODE
namespace async {

namespace internal {

template <typename T> struct task_handle_coro_core {
  public:
    struct join_awaitable {
        join_awaitable(task_handle_coro_core<T> &core) : _core(core) {}

        bool await_ready() { return _core.notified(); }

        void await_suspend(std::coroutine_handle<> h) {
            _core.add_waiting(h, this);
        }

        void await_resume() {}

      private:
        friend task_handle_coro_core<T>;
        task_handle_coro_core<T> &_core;
    };

    struct result_awaitable {
        result_awaitable(task_handle_coro_core<T> &core) : _core(core) {}

        bool await_ready() {
            if (auto v = std::move(_core.try_ready())) {
                _value = std::move(v);
                return true;
            } else {
                return false;
            }
        }

        void await_suspend(std::coroutine_handle<> h) {
            _core.add_waiting(h, this);
        }

        T await_resume() { return std::move(_value.value()); }

      private:
        friend task_handle_coro_core<T>;
        std::optional<T> _value;
        task_handle_coro_core<T> &_core;
    };

  private:
    using awaitables_t = std::variant<join_awaitable *, result_awaitable *>;
    struct waiting_values {
        coro_handle handle;
        awaitables_t awaitable;
    };

  public:
    task_handle_coro_core(
        internal::utils::buffered_observer_signal<void(void *)>
            &notify_signal) {
        _conn = notify_signal.connect([this](void *o) { notify(o); });
    };

    // needs to be locked
    bool notified() {
        std::lock_guard lck(_valueM);
        return _notified;
    }

    std::optional<T> try_ready() {
        std::lock_guard lck(_valueM);
        if (_notified) {
            return std::move(*static_cast<T *>(_value_ptr));
        }
        return std::nullopt;
    }

    void add_waiting(coro_handle h, const awaitables_t &awaitable) {
        std::lock_guard lck(_valueM);
        _waiting.emplace_back(h, awaitable);
    }

    void notify(void *o) {
        std::lock_guard lck(_valueM);
        for (const auto &waiter : _waiting) {
            if (std::holds_alternative<result_awaitable *>(waiter.awaitable)) {
                // CAN HAPPEN ONLY ONCE, more than this, UB
                auto awaitable = std::get<result_awaitable *>(waiter.awaitable);
                awaitable->_value = std::move(*static_cast<T *>(o));
                // spdlog::warn("AWAITABLE");
            }
            // spdlog::warn("RESUME");
            runtime::inst().submit_resume(waiter.handle);
        }
        _value_ptr = o;
        _notified = true;
    }

  private:
    std::mutex _valueM;
    void *_value_ptr{nullptr};
    bool _notified{false};
    std::list<waiting_values> _waiting;
    utils::scoped_signal_connection _conn;
};

template <> struct task_handle_coro_core<void> {
  public:
    struct join_awaitable {
        join_awaitable(task_handle_coro_core<void> &core) : _core(core) {}

        bool await_ready() { return _core.notified(); }

        void await_suspend(std::coroutine_handle<> h) {
            _core.add_waiting(h, this);
        }

        void await_resume() {}

      private:
        friend task_handle_coro_core<void>;
        task_handle_coro_core<void> &_core;
    };

  private:
    struct waiting_values {
        coro_handle handle;
        join_awaitable *awaitable;
    };

  public:
    task_handle_coro_core(
        internal::utils::buffered_observer_signal<void(void *)>
            &notify_signal) {
        _conn = notify_signal.connect([this](void *o) { notify(o); });
    };

    // needs to be locked
    bool notified() {
        std::lock_guard lck(_valueM);
        return _notified;
    }

    void add_waiting(coro_handle h, join_awaitable *awaitable) {
        std::lock_guard lck(_valueM);
        _waiting.emplace_back(h, awaitable);
    }

    void notify(void *o) {
        std::lock_guard lck(_valueM);
        for (const auto &waiter : _waiting) {
            runtime::inst().submit_resume(waiter.handle);
        }
        _value_ptr = o;
        _notified = true;
    }

  private:
    std::mutex _valueM;
    void *_value_ptr{nullptr};
    bool _notified{false};
    std::list<waiting_values> _waiting;
    utils::scoped_signal_connection _conn;
};
}; // namespace internal

template <typename T> struct task_handle : public internal::task_handle_base {
  private:
    task_handle(cid_t id) : internal::task_handle_base(id), notify_signal(10) {}

    T obj;
    void on_result(void *o) override {
        obj = std::move(*static_cast<T *>(o));
        notify_signal(&obj);
    }

    friend internal::runtime;

  public:
    struct thread_view {
        friend task_handle<T>;

      private:
        thread_view(internal::utils::buffered_observer_signal<void(void *)>
                        &notify_signal) {
            _conn = notify_signal.connect([this](void *o) { on_notify(o); });
        }

      public:
        void join() {
            std::unique_lock lck(_valueM);
            _valueCV.wait(lck, [this] { return _notified; });
        };

        [[nodiscard]] T result() {
            std::unique_lock lck(_valueM);
            _valueCV.wait(lck, [this] { return _notified && _value_ptr; });

            return std::move(*static_cast<T *>(_value_ptr));
        }

        [[nodiscard]] std::optional<T> try_result() {
            std::lock_guard lck(_valueM);
            if (_notified && _value_ptr) {
                return std::move(*static_cast<T *>(_value_ptr));
            }
            return std::nullopt;
        };

        void on_notify(void *o) {
            {
                std::lock_guard lck(_valueM);
                _value_ptr = o;
                _notified = true;
            }
            _valueCV.notify_one();
        }

      private:
        internal::utils::scoped_signal_connection _conn;
        void *_value_ptr{nullptr};
        bool _notified{false};
        std::mutex _valueM;
        std::condition_variable _valueCV;
    };

    struct coro_view {
        friend task_handle<T>;

      private:
        coro_view(internal::utils::buffered_observer_signal<void(void *)>
                      &notify_signal)
            : _coro_core(notify_signal) {}

      public:
        [[nodiscard]] internal::task_handle_coro_core<T>::join_awaitable
        join_coro() {
            return {_coro_core};
        }
        [[nodiscard]] internal::task_handle_coro_core<T>::result_awaitable
        result_coro() {
            return {_coro_core};
        }

      private:
        internal::task_handle_coro_core<T> _coro_core;
    };

    thread_view thread() { return {notify_signal}; }
    coro_view coro() { return {notify_signal}; }

  private:
    internal::utils::buffered_observer_signal<void(void *)> notify_signal;
};

template <> struct task_handle<void> : public internal::task_handle_base {
  private:
    task_handle(cid_t id) : internal::task_handle_base(id), notify_signal(10) {}

    void on_result(void *o) override { notify_signal(o); }

    friend internal::runtime;

  public:
    struct thread_view {
        friend task_handle<void>;

      private:
        thread_view(internal::utils::buffered_observer_signal<void(void *)>
                        &notify_signal) {
            _conn = notify_signal.connect([this](void *o) { on_notify(o); });
        }

      public:
        void join() {
            std::unique_lock lck(_valueM);
            _valueCV.wait(lck, [this] { return _notified; });
        };

        void on_notify(void *o) {
            {
                std::lock_guard lck(_valueM);
                _value_ptr = o;
                _notified = true;
            }
            _valueCV.notify_one();
        }

      private:
        internal::utils::scoped_signal_connection _conn;
        void *_value_ptr{nullptr};
        bool _notified{false};
        std::mutex _valueM;
        std::condition_variable _valueCV;
    };

    struct coro_view {
        friend task_handle<void>;

      private:
        coro_view(internal::utils::buffered_observer_signal<void(void *)>
                      &notify_signal)
            : _coro_core(notify_signal) {}

      public:
        [[nodiscard]] internal::task_handle_coro_core<void>::join_awaitable
        join_coro() {
            return {_coro_core};
        }

      private:
        internal::task_handle_coro_core<void> _coro_core;
    };

    thread_view thread() { return {notify_signal}; }
    coro_view coro() { return {notify_signal}; }

  private:
    internal::utils::buffered_observer_signal<void(void *)> notify_signal;
};
} // namespace async
