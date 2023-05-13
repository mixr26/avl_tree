#ifndef AVL_TREE_H
#define AVL_TREE_H


#include <iostream>

#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>

namespace avl {

template <typename Key, typename T = Key, typename Cmp = std::less<Key>>
class avl_tree final {
public:
    using size_type = std::size_t;
    using balance_type = int8_t;
    using key_type = Key;
    using val_type = T;
    using cmp_type = Cmp;

private:
    struct Node final {
        key_type _key;
        val_type _value;
        balance_type _balance_factor{0};

        Node *_parent{nullptr};
        std::unique_ptr<Node> _left{nullptr};
        std::unique_ptr<Node> _right{nullptr};

        explicit Node(key_type key, val_type value, Node *parent) : _key(key), _value(value), _parent(parent) {}
        Node(const Node &) = default;
        Node(Node &&) = default;
        Node &operator=(const Node &) = default;
        Node &operator=(Node &&) = default;
        ~Node() { std::cout << "delete key: " << _key << std::endl; }
    };

    using node_type = Node;

    size_type _size{0};
    const cmp_type _comparator{};
    std::unique_ptr<node_type> _root{nullptr};

    node_type *insert_internal(node_type *insert, const key_type &key, const val_type &value) {
        assert(insert && "Inserting at nullptr.");

        if (_comparator(key, insert->_key)) {
            if (insert->_left == nullptr) {
                insert->_left = std::make_unique<node_type>(key, value, insert);
                ++_size;
                return insert->_left.get();
            } else
                return insert_internal(insert->_left.get(), key, value);
        } else {
            if (insert->_right == nullptr) {
                insert->_right = std::make_unique<node_type>(key, value, insert);
                ++_size;
                return insert->_right.get();
            } else {
                return insert_internal(insert->_right.get(), key, value);
            }
        }
    }

    std::unique_ptr<node_type> &get_unique_ptr(node_type *node) noexcept {
        auto *parent = node->_parent;
        if (parent != nullptr) {
            return node == parent->_left.get() ? parent->_left : parent->_right;
        } else
            return _root;
    }

    void rotate_subtree_left(std::unique_ptr<node_type> &old_root) noexcept {
        auto *old_root_parent = old_root->_parent;
        auto *new_root = old_root->_right.get();
        std::swap(old_root->_right, new_root->_left);
        std::swap(old_root, new_root->_left);
        new_root->_parent = old_root_parent;
        new_root->_left->_parent = new_root;
        if (new_root->_left->_right)
            new_root->_left->_right->_parent = new_root->_left.get();
        new_root->_balance_factor = 0;
        new_root->_left->_balance_factor = 0;
    }

    void rotate_subtree_right(std::unique_ptr<node_type> &old_root) noexcept {
        auto *old_root_parent = old_root->_parent;
        auto *new_root = old_root->_left.get();
        std::swap(old_root->_left, new_root->_right);
        std::swap(old_root, new_root->_right);
        new_root->_parent = old_root_parent;
        new_root->_right->_parent = new_root;
        if (new_root->_right->_left)
            new_root->_right->_left->_parent = new_root->_right.get();
        new_root->_balance_factor = 0;
        new_root->_right->_balance_factor = 0;
    }

    void retrace(std::unique_ptr<node_type> &node) {
        auto *parent = node->_parent;
        if (!parent)
            return;

        const auto *left_child = parent->_left.get();
        const auto *right_child = parent->_right.get();
        if (node.get() == right_child) {
            if (parent->_balance_factor > 0) {
                if (node->_balance_factor >= 0)
                    rotate_subtree_left(get_unique_ptr(parent));
                return;
            } else if (parent->_balance_factor < 0) {
                parent->_balance_factor = 0;
                return;
            } else {
                ++parent->_balance_factor;
                return retrace(get_unique_ptr(parent));
            }
        } else {
            assert(node.get() == left_child);
            if (parent->_balance_factor < 0) {
                if (node->_balance_factor <= 0)
                    rotate_subtree_right(get_unique_ptr(parent));
                return;
            } else if (parent->_balance_factor > 0) {
                parent->_balance_factor = 0;
                return;
            } else {
                --parent->_balance_factor;
                return retrace(get_unique_ptr(parent));
            }
        }
    }

public:
    avl_tree() = default;

    bool empty() const noexcept { return _root == nullptr; }
    bool size() const noexcept { return _size; }
    constexpr size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

    void clear() noexcept { _root.reset(); _size = 0; }
    [[maybe_unused]] std::pair<node_type *, bool> insert(const key_type &key, const val_type &value) {
        if (!_root) {
            _root = std::make_unique<node_type>(key, value, nullptr);
            ++_size;
            return std::pair(_root.get(), true);
        }

        if (_size == max_size())
            return std::pair(nullptr, false);

        auto *new_node = insert_internal(_root.get(), key, value);
        retrace(get_unique_ptr(new_node));

        return std::pair(new_node, true);
    }
    [[maybe_unused]] std::pair<node_type *, bool> insert(const val_type &value) { return insert(value, value); }

    bool operator==(const avl_tree<Key, T, Cmp> &lhs, const avl_tree<Key, T, Cmp> &rhs) const noexcept {

    }

    void dump(const node_type *node) {
        if (!node)
            return;

        dump(node->_left.get());
        std::cout << node->_key << " " << (int32_t)node->_balance_factor << std::endl;
        dump(node->_right.get());
    }
    void dump() { std::cout << "root: " << _root->_key << std::endl; dump(_root.get()); }
};

} // end namespace avl


#endif // AVL_TREE_H
