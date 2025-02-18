#pragma once

#include <algorithm>
#include <channel_test.h>
#include <memory>
#include <pch.h>
#include <runtime/runtime_core.h>

namespace async {

struct select_awaitable {
    using handler_func = std::function<void(std::coroutine_handle<>)>;

    select_awaitable(bool on_ready, handler_func &&func)
        : _on_ready(on_ready), _handler(std::move(func)) {}

    bool await_ready() { return _on_ready; }

    void await_suspend(std::coroutine_handle<> h) { _handler(h); }

    void await_resume() {}

    handler_func _handler;
    bool _on_ready;
};

template <typename... Args>
struct select_core : public std::enable_shared_from_this<select_core<Args...>> {
    using variant_types = std::variant<Args...>;

    struct select_value_getter {
      public:
        select_value_getter(select_core &core) : _core(core) {}

        select_awaitable wait() {
            const auto resolver = [this](std::coroutine_handle<> h) {
                _core.add_waiting(h);
            };

            const auto empty_resolver = [](std::coroutine_handle<>) {};

            for (auto &channel : _core._channels) {
                if (_value = channel->try_fetch(); _value) {
                    return select_awaitable{true, empty_resolver};
                }
            }

            return select_awaitable{false, resolver};
        }

        select_awaitable wait_for(duration_t duration) {}

        std::optional<variant_types> get() {
            if (!_value) {
                return std::nullopt;
            }

            // if nullopt notify that the type isn't valid
            // somehow
            // introduce error handling, logging etc...
            return extract_type(_value.value());
        }

      private:
        [[nodiscard]] constexpr inline auto extract_type(std::any value) {
            return (
                [&value] {
                    if (value.type() == typeid(Args)) {
                        return std::any_cast<Args>(value);
                    } else {
                        return std::nullopt;
                    }
                }(),
                ...);
        }

        [[nodiscard]] bool resolved() { return _value != std::nullopt; }

        void set_value(std::any value) { _value = value; }

        friend select_core;

        select_core &_core;
        std::optional<std::any> _value{std::nullopt};
    };

    select_core() : _id(_s_id_counter.fetch_add(1)) {}

    struct waiting_block {
        std::coroutine_handle<> h;
        std::shared_ptr<select_value_getter> value;
    };

    void attach_channels(
        std::initializer_list<std::shared_ptr<channel_base>> &channels) {
        _channels = {channels.begin(), channels.end()};
    }

    void attach_observers() {
        std::ranges::for_each(_channels, [this](auto &chan) {
            chan->add_obeservable(
                _id, [this](std::any value) { notify_finished(value); });
        });
    }

    void notify_finished(std::any value) {
        auto waiting = _waiting.front();
        _waiting.pop_back();

        waiting->value.set_value(value);

        runtime::runtime::get().submit([waiting] { waiting->h.resume(); }, 1);
    }

    void add_waiting(std::coroutine_handle<> h,
                     s_ptr<select_value_getter> getter) {
        _waiting.emplace_back(h, getter);
    }

    void add_timed_wait(std::coroutine_handle<> h,
                        s_ptr<select_value_getter> getter,
                        duration_t duration) {
        waiting_block waiting_block;

        {
            waiting_block = {h, getter};
            _waiting.push_back(waiting_block);
        }

        runtime::runtime::get().attach_timer(duration, false, [waiting_block] {
            if (waiting_block.value->resolved()) {
                return;
            }

            runtime::runtime::get().submit(
                [waiting_block] { waiting_block.h.resume(); });
        });
    };

  private:
    static inline std::atomic<cid_t> _s_id_counter{0};
    cid_t _id;
    std::deque<waiting_block> _waiting;
    std::vector<std::shared_ptr<channel_base>> _channels;
};

// TODO: invalid ref, invalid pointers
template <typename... ChannelTypes> struct co_select {
    using core_t = select_core<ChannelTypes...>;
    using value_getter_t = core_t::select_value_getter;

    co_select(std::initializer_list<std::shared_ptr<channel_base>> channels) {
        _core.attach_channels(channels);
    }

    core_t::select_value_getter fetch() {
        _core.attach_observers();
        return std::make_shared<value_getter_t>(_core);
    }

  private:
    core_t _core;
};
} // namespace async
