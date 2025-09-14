#pragma once

#include <async/internal/pch.h>

namespace async::internal {

// sync is expected to be handled by the caller
// this of another use case for when we allow reallocation/resize
// only works where hash is an injection function, has to be unique
// we don't handle hash collisions
template <typename K, typename V> class associative_sparse_set {
    using cont_type = std::vector<V>;
    using size_t = cont_type::size_type;

  public:
    // TODO: can this be made into a generic building block?
    class iterator {
      private:
        V *ptr;

      public:
        explicit iterator(V *p) : ptr(p) {}

        V &operator*() { return *ptr; }

        iterator &operator++() {
            ++ptr;
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator &other) const {
            return ptr == other.ptr;
        }

        bool operator!=(const iterator &other) const {
            return ptr != other.ptr;
        }
    };

  public:
    associative_sparse_set(std::size_t fixed_size) {
        _data.resize(fixed_size);
        _max = fixed_size;
    }

    void add(const K &key, const V &value) {
        if (!has_available()) {
            return;
        }

        // fun stuff can be done here!
        _mp[key] = _current;
        _data[_current] = value;

        _current++;
    }

    void add(const K &key, V &&value) {
        if (!has_available()) {
            return;
        }

        _mp[key] = _current;
        _data[_current] = std::move(value);

        _current++;
    }

    // the only caveat beeing that the desctrutor is defered
    // when we overwrite the data
    bool remove(const K &key) {
        if (auto i = _mp.find(key); i != std::end(_mp)) {
            const auto idx = i->second;

            // swapping is leading to unnecessary copying i think!
            std::swap(_data[idx], _data[_current - 1]);

            _mp.erase(i);
            _current--;

            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] bool has_available() const { return _current < _max; }
    [[nodiscard]] size_t size() const { return _data.size(); }

    [[nodiscard]] std::optional<K *> find(const K &key) const {
        if (auto i = _mp.find(key); i != std::end(_mp)) {
            auto idx = i->second;

            return &_data[idx];
        }

        return std::nullopt;
    }

    // for now just for testing
    void for_each(std::function<void(const V &)> f) {
        for (int i = 0; i < _current; i++) {
            f(_data[i]);
        }
    }

    [[nodiscard]] iterator begin() { return iterator(&_data[0]); }
    // end might not be correct when _current == _max
    [[nodiscard]] iterator end() { return iterator(&_data[_current]); }

    template <typename A, typename B>
    constexpr bool operator<(const associative_sparse_set<A, B> &set) const {
        return _current < set._current;
    }

    [[nodiscard]] const K &least_key() {
        if (_mp.size() < 1) {
            // handle error better
            return {};
        }

        const auto least = _mp.begin();
        return least.second;
    }

    [[nodiscard]] const K &greatest_key() {
        if (_mp.size() < 1) {
            // handle error better
            return {};
        }

        auto least = _mp.end();
        std::advance(least, -1);

        return least.second;
    }

  private:
    std::vector<K> _data;
    std::map<K, size_t> _mp;

    size_t _max;
    size_t _current{0};
};

} // namespace async::internal
