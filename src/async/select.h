#pragma once

#include "async/defines.h"
#include "async/runtime/runtime.h"
#include <async/channel.h>
#include <async/pch.h>
#include <async/runtime/runtime_core.h>

#include <iostream>

namespace async {

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
                /*std::cout << "EQUALS  " << k << "\n";*/
                /*std::cout << "VALUE TYPE " << value.type().name() << "\n";*/
                /*std::cout << "TYPE " << typeid(Args).name() << "\n";*/

                if (value.type() == typeid(Args)) {
                    extracted = std::optional<variant_types>(
                        std::any_cast<Args>(value));
                }
            }(),
            ...);

        return extracted;
    }

    /*std::optional<variant_types> assign_value(std::any value) {*/
    /*    auto k = extract_type(value);*/
    /**/
    /*    if (k == std::nullopt) {*/
    /*        return std::nullopt;*/
    /*    }*/
    /**/
    /*    return k;*/
    /*}*/
    /**/
    std::optional<variant_types> try_fetch() {
        // Iterate the map starting by the first, by default sorting done on
        // readyness and id(can be more)
        // first should be ready if not then we have O(n) time
        // worst case O(n)
        if (!_value_queue.empty()) {
            auto v = _value_queue.front();
            _value_queue.pop_back();
            return extract_type(v);
        }

        // Don't iterate over all of them, keep a list of ready, already
        // implemented just use the observer mechanic if no one is waiting
        // if not found, then iterate, maybe the iteration can be also optimized
        for (auto &channel : _channels) {
            if (auto value = channel->try_fetch(); value) {
                auto k = value.value();

                /*std::cout << "FROM TRY FETCH " << k.type().name() << "\n";*/

                return extract_type(k);
            }
        }

        return std::nullopt;
    }

    void attach_channels(std::shared_ptr<channel<Args>>... channels) {
        ((_channels.push_back(channels)), ...);

        // maybe no need for copy, think, line 122
        attach_observers();
    }

    void attach_observers() {
        // this should be called only once at start up, keep a list of the
        // values
        //

        auto channels = _channels;

        std::ranges::for_each(channels, [this](auto &chan) {
            // reordering while adding might be complicated, find a way to to
            // this
            chan->add_obeservable(
                _id, [this](std::any value) { notify_on_value_state(value); });
        });
    }

    void notify_on_value_state(value_state v_state, cid_t id, std::any value) {
        // TODO: THINK ABOUT locking
        std::coroutine_handle<> h = nullptr;

        // switch on type, re order channels by readyness, first by default if
        // not consumed always ready, check move to next in tree
        // should have access to channel pointer to try fetch value
        // channel handles extra cases if early consumption before we get to the
        // value, notify every change, keep track even if we don't need to
        // receive the value keep track if the value is consumed, maybe counter
        // to know if we have a valid value if not we don't iterate
        // maybe add edge triggered level triggered
        {
            std::lock_guard lck(_waiting_M);
            // TODO log n
            auto empty_wait_it =
                std::ranges::find_if(_waiting, [](auto &waiting) {
                    return !waiting.awaitable->_value.has_value();
                });

            if (empty_wait_it != std::end(_waiting)) {
                empty_wait_it->awaitable->_value = extract_type(value);
                std::cout << "\tNOTIFIED GOT VALUE\n";
                h = empty_wait_it->h;
            } else {
                _value_queue.push_back(value);
            }
        }

        if (h) {
            runtime::runtime::get().submit_resume(h);
        }
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

        runtime::runtime::get().attach_timer(duration, false, [block] {
            bool is_handle_valid = block.h;

            if (!is_handle_valid || block.awaitable._value) {
                return;
            }

            runtime::runtime::get().submit_resume(block.h);
        });
    };

  private:
    static inline std::atomic<cid_t> _s_id_counter{0};
    cid_t _id;

    std::mutex _waiting_M;
    // TODO: think about this !
    std::deque<waiting_block> _waiting;

    std::deque<std::any> _value_queue;
    std::vector<s_ptr<channel_base>> _channels;
};

// TODO: invalid ref, invalid pointers
// get the types from the channels, store only the channel base or thye observer
template <typename... ChannelTypes> struct co_select {
    using variant_type = std::variant<ChannelTypes...>;
    using core_t = select_core<ChannelTypes...>;
    using select_awaitable_t = core_t::select_awaitable;

    co_select(std::shared_ptr<channel<ChannelTypes>>... channels) {
        _core.attach_channels(channels...);
    }

    void add_channel();

    select_awaitable_t fetch() { return select_awaitable_t{_core}; }

    std::optional<variant_type> rtn_type() { return std::nullopt; }

  private:
    core_t _core;
};
} // namespace async
