
#include <async/internal/util/observer_signal.h>

namespace async::internal {

enum class select_kind { ONE_SHOT, PERSISTENT };

struct select_notify_base;

using select_notify_func =
    std::function<bool(select_notify_base *, value_state)>;

struct select_notify_base {
  public:
    u64 id() { return (u64)this; }

    virtual bool is_ready() = 0;
    virtual const std::type_info &type() = 0;

    virtual std::pair<utils::signal_connection, value_state>
    add_obeservable(select_kind, select_notify_func &&func) = 0;
};

using select_handle = internal::select_notify_base *;

struct sequential_match_select {
    friend struct select_core;

  private:
    sequential_match_select(select_handle handle) : _handle(handle) {}

  public:
    template <typename T, typename F>
        requires std::invocable<F, T &>
    void on(T &o, F &&callback) {
        select_handle s = &o;

        if (_handle == s) {
            callback(o);
        }
    }

    template <typename T, typename F>
        requires std::invocable<F>
    void on(T &o, F &&callback) {
        select_handle s = &o;

        if (_handle == s) {
            callback();
        }
    }

    template <typename T, typename F>
        requires std::invocable<F, T &>
    void on(std::shared_ptr<T> &o, F &&callback) {
        on(*o, callback);
    }

    template <typename T, typename F>
        requires std::invocable<F>
    void on(std::shared_ptr<T> &o, F &&callback) {
        on(*o, callback);
    }

  private:
    select_handle _handle;
};
} // namespace async::internal
