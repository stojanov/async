#pragma once

#include <async/pch.h>
#include <iostream>

namespace async {

// sync is expected to be handled by the caller
// this of another use case for when we allow reallocation/resize
// only works where hash is an injection function, has to be unique
// we don't handle hash collisions
template <typename T, typename H_f = std::hash<T>>
class associative_sparse_set {
    using cont_type = std::vector<T>;
    using size_t = cont_type::size_type;
    static constexpr auto hasher = H_f{};

  public:
    // TODO: can this be made into a generic building block?
    class iterator {
      private:
        T *ptr;

      public:
        explicit iterator(T *p) : ptr(p) {}

        T &operator*() { return *ptr; }

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

    void add(const T &value) {
        if (!has_available()) {
            return;
        }

        // fun stuff can be done here!
        _mp[hasher(value)] = _current;
        _data[_current] = value;

        _current++;
    }

    void add(T &&value) {
        if (!has_available()) {
            return;
        }

        _mp[hasher(value)] = _current;
        _data[_current] = std::move(value);

        _current++;
    }

    // the only caveat beeing that the desctrutor is defered
    // when we overwrite the data
    bool remove_hash(std::size_t hash_value) {
        if (auto i = _mp.find(hash_value); i != std::end(_mp)) {
            const auto idx = i->second;

            std::swap(_data[idx], _data[_current - 1]);

            _mp.erase(i);
            _current--;

            return true;
        } else {
            return false;
        }
    }

    bool remove(const T &value) { return remove_hash(hasher(value)); }

    [[nodiscard]] bool has_available() const { return _current != _max; }
    [[nodiscard]] size_t size() const { return _data.size(); }

    [[nodiscard]] std::optional<T *> find(const T &value) const {
        return find_hash(hasher(value));
    }

    [[nodiscard]] std::optional<T *> find_hash(std::size_t hash_value) const {
        if (auto i = _mp.find(hash_value); i != std::end(_mp)) {
            return &_data[i->second];
        } else {
            return std::nullopt;
        }
    }

    // for now just for testing
    void for_each(std::function<void(const T &)> f) {
        for (int i = 0; i < _current; i++) {
            f(_data[i]);
        }
    }

    // this is arguable, maybe when i modify the map to hold this as well
    // untill then we cannot guarantee that for every index we have the correct
    // previous one they might get invalidated
    [[nodiscard]] std::optional<T> find_idx(size_t idx) const {
        return std::nullopt;
    }
    bool remove_idx(size_t idx) { return false; }

    iterator begin() { return iterator(&_data[0]); }
    iterator end() { return iterator(&_data[_current]); }

    template <typename A, typename B>
    constexpr bool operator<(const associative_sparse_set<A, B> &set) const {
        return _current < set._current;
    }

  private:
    std::vector<T> _data;
    std::unordered_map<std::size_t, size_t> _mp;

    size_t _max;
    size_t _current{0};
};

} // namespace async
