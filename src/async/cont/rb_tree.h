#pragma once

namespace async {

enum class Color { RED, BLACK };

template <typename T> struct Node {
    T value; // support emplace
    Color color;
    Node<T> *left;
    Node<T> *right;
    Node<T> *parent;

    template <typename... Args>
    static Node create_and_emplace(Color cl, Args &&...args) {
        return new Node{{std::forward<Args>(args)...}, cl};
    }

    static Node create(Color cl, const T &v) { return Node{v, cl}; }
    static Node create(Color cl, T &&v) { return Node{std::move(v), cl}; }
};

template <typename T> class rb_tree {
  public:
    using node_t = Node<T>;

    void insert(const T &value) {
        auto new_node = node_t::create(Color::RED, value);

        node_t *parent = nullptr;
        node_t *current = _root;

        while (current != nullptr) {
            parent = current;

            // sorter
            if (new_node->value < current->value) {
                current = current->left;
            } else {
                current = current->right;
            }
            //

            new_node.parent = parent;

            if (parent == nullptr) {

            } else if (new_node->value < parent->value) {
                parent->left = new_node;
            } else {
                parent->right = new_node;
            }
        }
    }

  private:
    node_t _root;
};

} // namespace async
