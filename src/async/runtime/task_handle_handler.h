#pragma once

#include <async/defines.h>
#include <async/pch.h>
#include <async/runtime/task_handle.h>

namespace async::internal {
class runtime_core;

// user/consumer/caller is responsible for sync access
class task_handle_handler {
  public:
    task_handle_handler(runtime_core &_core);

    template <typename T> s_ptr<task_handle<T>> create_task_handle(cid_t id) {
        if constexpr (std::same_as<T, void>) {
            return;
        }

        if (!_meta_notify.contains(typeid(T))) {
            _meta_notify[typeid(T)] = [this](cid_t id, void *o) {
                notify_of_result<T>(id, o);
            };
        }
        auto t_handle = std::shared_ptr<task_handle<T>>(new task_handle<T>(id));
        _task_handles.insert(std::pair{id, t_handle});
    }

    void notify_of_result(std::type_index type_idx, cid_t id, void *o) {
        const auto &callback = _meta_notify.at(type_idx);
        callback(id, o);
        //
        auto i = _task_handles.find(id);

        // i->second->on_result(o);
    }

  private:
    template <typename T> void notify_of_result() {}

  private:
    runtime_core &_core;
    std::map<cid_t, s_ptr<task_handle_base>> _task_handles;
    std::map<std::type_index, std::function<void(cid_t, void *o)>> _meta_notify;
};
} // namespace async::internal
