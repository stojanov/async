#pragma once

#include <algorithm>
#include <async/channel.h>
#include <async/cont/midpoint_map.h>
#include <async/defines.h>
#include <async/pch.h>
#include <async/runtime/runtime.h>
#include <async/runtime/runtime_core.h>

#include <iostream>
#include <spdlog/spdlog.h>

namespace async {

namespace internal {
struct channel_wrapper {
    s_ptr<internal::channel_base> channel;
    value_state state;
};

struct channel_sort {
    u32 prio(const channel_wrapper &ch) { return static_cast<u32>(ch.state); }
};

template <typename... Args> struct select_core {
    using core_t = select_core<Args...>;
    using variant_types = std::variant<Args...>;

    struct select_awaitable {
        select_awaitable(core_t &core) : _core(core) {}

        bool await_ready() {
            if (_value = _core.try_fetch(); _value) {
                std::cout << "\tTRY_FETCH GOT VALUE\n";
                return true;
            }
            return false;
        }

        void await_suspend(std::coroutine_handle<> h) {
            if (_value) {
                h.resume();
            } else {
                _h = h;
                _core.add_waiting(h, this);
            }
        }

        std::optional<variant_types> await_resume() {
            _core.clean_up_waiting(_h);
            return _value;
        }

        core_t &_core;
        std::coroutine_handle<> _h;
        std::optional<variant_types> _value;
    };

    select_core() : _id(_s_id_counter.fetch_add(1)) {}

    struct waiting_block {
        std::coroutine_handle<> h;
        select_awaitable *awaitable;
    };

    [[nodiscard]] std::optional<variant_types> extract_type(std::any value) {
        std::optional<variant_types> extracted = std::nullopt;

        (
            [&value, &extracted]() {
                // std::cout << "VALUE TYPE " << value.type().name() << "\n";
                // std::cout << "TYPE " << typeid(Args).name() << "\n";

                if (value.type() == typeid(Args)) {
                    extracted = std::optional<variant_types>(
                        std::any_cast<Args>(value));
                }
            }(),
            ...);

        return extracted;
    }

    std::optional<variant_types> try_fetch() {
        const auto prio = _chnl.get_all_latter();

        for (auto i = prio.first; i != prio.second; i++) {
            if (auto value = std::move(i->second->try_fetch())) {
                spdlog::warn("GOT VALUE FROM TRY FETCH ");
                return extract_type(value.value());
            }
        }

        return std::nullopt;
    }

    void attach_channels(std::shared_ptr<channel<Args>>... channels) {
        ((_chnl.add(channels->id(), channels, 1)), ...);

        // maybe no need for copy, think, line 110
        attach_observers();
    }

    void attach_observers() {
        // TODO: find a way to remove this,
        // notify can reorder the map, making the iterators invalid
        auto channels = _chnl;

        for (auto i = channels.begin(); i != channels.end(); i++) {
            i->second->add_obeservable(
                _id, [this](value_state v_state, cid_t id,
                            std::optional<std::any> value) {
                    return notify_on_value_state(v_state, id, value);
                });
        }
    }

    bool notify_on_value_state(value_state v_state, cid_t id,
                               std::optional<std::any> value) {
        // TODO: THINK ABOUT locking
        std::coroutine_handle<> h = nullptr;

        switch (v_state) {
        case value_state::READY:
        case value_state::CONSUMED:
            _chnl.update_prio(id, (u32)v_state);
            return false;
        case value_state::NOTIFY: {
            std::lock_guard lck(_waiting_M);
            if (_waiting.empty()) {
                return false;
            }

            auto empty_wait_it =
                std::ranges::find_if(_waiting, [](auto &waiting) {
                    return !waiting.awaitable->_value.has_value();
                });

            if (empty_wait_it != std::end(_waiting)) {
                empty_wait_it->awaitable->_value = extract_type(value.value());
                h = empty_wait_it->h;

                spdlog::warn("GOT VALUE FROM NOTIFY {}",
                             value.value().type().name());
            }
        }
        default:
            break;
        }

        if (h) {
            internal::runtime::runtime::inst().submit_resume(h);
            return true;
        }

        return false;
    }

    void clean_up_waiting(std::coroutine_handle<> h) {
        std::lock_guard lck(_waiting_M);
        std::erase_if(_waiting, [h](auto &waiting) { return waiting.h == h; });
    }

    void add_waiting(std::coroutine_handle<> h, select_awaitable *awaitable) {
        std::lock_guard lck(_waiting_M);
        _waiting.emplace_back(h, awaitable);
    }

    void add_timed_wait(std::coroutine_handle<> h, select_awaitable *awaitable,
                        duration_t duration) {
        waiting_block block{h, awaitable};

        {
            std::lock_guard lck(_waiting_M);
            _waiting.push_back(block);
        }

        internal::runtime::inst().attach_timer(duration, false, [block] {
            // TODO: can't rely on handle not beeing valid
            if (!block.h || block.awaitable._value) {
                return;
            }

            internal::runtime::inst().submit_resume(block.h);
        });
    };

  private:
    static inline std::atomic<cid_t> _s_id_counter{1};
    cid_t _id;

    std::mutex _waiting_M;
    // TODO: think about this !
    std::deque<waiting_block> _waiting;

    // std::vector<s_ptr<channel_base>> _channels;
    std::deque<std::any> _value_queue;

    midpoint_map<s_ptr<internal::channel_base>> _chnl;
};
}; // namespace internal

// TODO: invalid ref, invalid pointers
// get the types from the channels, store only the channel base or thye observer
template <typename... ChannelTypes> struct co_select {
    using variant_type = std::variant<ChannelTypes...>;
    using core_t = internal::select_core<ChannelTypes...>;
    using select_awaitable_t = core_t::select_awaitable;

    co_select(std::shared_ptr<channel<ChannelTypes>>... channels) {
        _core.attach_channels(channels...);
    }

    select_awaitable_t fetch() { return select_awaitable_t{_core}; }

    // to be used with decltype
    std::optional<variant_type> rtn_type() { return std::nullopt; }

  private:
    core_t _core;
};
} // namespace async
