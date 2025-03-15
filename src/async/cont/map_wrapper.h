#pragma once

#include <functional>
#include <map>

namespace async {

namespace {
template <typename T> using KeyAdapterFunc = T (*)(const T &);
template <typename T> using CompareFunc = int (*)(const T &);
}; // namespace

// TODO: concepts
template <typename Key, typename Value, typename Adapter = std::identity,
          typename Compare = std::less<Key>, bool Synced = false>
class rb_map {
    using map_type = std::map<Key, Value, Compare>;
    using it_type = map_type::iterator;

  public:
    void add(const Key &key, const Value &value) {
        _map.insert(std::make_pair<Key, Value>(Adapter(key), value));
    }

    void add(const Key &key, Value &&value) {
        _map.insert(std::make_pair<Key, Value>(Adapter(key), std::move(value)));
    }

    bool remove(const Key &key) { _map.erase(key); }

    it_type find(const Key &key) {
        /*
         * key + factor
         *
         *
         * */

        /*
         * sorting
         * ready_factor key
         *
         * */
        _map.find(key);
    }

    it_type begin() { return _map.begin(); }
    it_type end() { return _map.end(); }

  private:
    map_type _map;
};

}; // namespace async
