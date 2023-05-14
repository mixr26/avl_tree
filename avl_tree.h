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
    using node_val_type = std::pair<key_type, val_type>;
    using cmp_type = Cmp;

private:
    struct Node final {
        node_val_type _value{};
        balance_type _balance_factor{0};

        Node *_parent{nullptr};
        std::unique_ptr<Node> _left{nullptr};
        std::unique_ptr<Node> _right{nullptr};

        Node() = default;
        explicit Node(const node_val_type &value, Node *parent) : _value(value), _parent(parent) {}
        Node(const Node &) = default;
        Node(Node &&) = default;
        Node &operator=(const Node &) = default;
        Node &operator=(Node &&) = default;
    };

    using node_type = Node;

    size_type _size{0};
    const cmp_type _comparator{};
    std::unique_ptr<node_type> _root_sentinel{nullptr};
    node_type *_begin{nullptr};

    node_type *root() noexcept { return _root_sentinel->_left.get(); }
    const node_type *root() const noexcept { return _root_sentinel->_left.get(); }

    node_type *insert_internal(node_type *insert, const node_val_type &value) {
        assert(insert && "Inserting at nullptr.");

        if (_comparator(value.first, insert->_value.first)) {
            if (insert->_left == nullptr) {
                insert->_left = std::make_unique<node_type>(value, insert);
                if (insert == _begin)
                    _begin = insert->_left.get();
                ++_size;
                return insert->_left.get();
            } else
                return insert_internal(insert->_left.get(), value);
        } else {
            if (insert->_right == nullptr) {
                insert->_right = std::make_unique<node_type>(value, insert);
                ++_size;
                return insert->_right.get();
            } else {
                return insert_internal(insert->_right.get(), value);
            }
        }
    }

    std::unique_ptr<node_type> &get_unique_ptr(node_type *node) noexcept {
        auto *parent = node->_parent;
        assert(parent != nullptr);
        return node == parent->_left.get() ? parent->_left : parent->_right;
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
        if (parent == _root_sentinel.get())
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
    template <typename ItT>
    struct Iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_cv_t<ItT>;
        using pointer = ItT *;
        using reference = ItT &;

        Iterator(node_type *ptr) : _ptr(ptr) {}

        reference operator*() const { return _ptr->_value; }
        pointer operator->() { return &(_ptr->_value); }

        Iterator<ItT> &operator++() { next(); return *this; }
        Iterator<ItT> operator++(int) {
            Iterator<ItT> tmp = *this;
            next();
            return tmp;
        }

        Iterator<ItT> &operator--() { prev(); return *this; }
        Iterator<ItT> operator--(int) {
            Iterator<ItT> tmp = *this;
            prev();
            return tmp;
        }

        friend bool operator==(const Iterator<ItT> &lhs, const Iterator<ItT> &rhs) { return lhs._ptr == rhs._ptr; }
        friend bool operator!=(const Iterator<ItT> &lhs, const Iterator<ItT> &rhs) { return !(lhs == rhs); }

    private:
        node_type *_ptr;

        node_type *smallest_subtree_elt(node_type *root) const noexcept {
            if (root->_left == nullptr)
                return root;
            return smallest_subtree_elt(root->_left.get());
        }

        node_type *largest_subtree_elt(node_type *root) const noexcept {
            if (root->_right == nullptr)
                return root;
            return largest_subtree_elt(root->_right.get());
        }

        void next() noexcept {
            if (_ptr->_right)
                _ptr = smallest_subtree_elt(_ptr->_right.get());
            else {
                while (_ptr != _ptr->_parent->_left.get())
                    _ptr = _ptr->_parent;
                _ptr = _ptr->_parent;
            }
        }

        void prev() noexcept {
            if (_ptr->_left)
                _ptr = largest_subtree_elt(_ptr->_left.get());
            else {
                while (_ptr != _ptr->_parent->_right.get())
                    _ptr = _ptr->_parent;
                _ptr = _ptr->_parent;
            }
        }
    };

    using iterator = Iterator<node_val_type>;
    using const_iterator = Iterator<const node_val_type>;

    iterator begin() noexcept { return iterator(_begin); }
    const_iterator cbegin() const noexcept { return const_iterator(_begin); }
    iterator end() noexcept { return iterator(_root_sentinel.get()); }
    const_iterator cend() const noexcept { return const_iterator(_root_sentinel.get()); }

    avl_tree() : _root_sentinel{std::make_unique<node_type>()}, _begin{_root_sentinel.get()} {}

    bool empty() const noexcept { return root() == nullptr; }
    bool size() const noexcept { return _size; }
    constexpr size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

    void clear() noexcept { _root_sentinel->_left.reset(nullptr); _size = 0; }
    [[maybe_unused]] std::pair<iterator, bool> insert(const node_val_type &value) {
        if (!root()) {
            _root_sentinel->_left = std::make_unique<node_type>(value, _root_sentinel.get());
            _begin = root();
            ++_size;
            return std::pair(iterator(root()), true);
        }

        if (_size == max_size())
            return std::pair(nullptr, false);

        auto *new_node = insert_internal(root(), value);
        retrace(get_unique_ptr(new_node));

        return std::pair(iterator(new_node), true);
    }
    [[maybe_unused]] std::pair<iterator, bool> insert(const val_type &value) { return insert(std::make_pair(value, value)); }

    void dump(const node_type *node) {
        if (!node)
            return;

        dump(node->_left.get());
        std::cout << node->_value.first << " " << (int32_t)node->_balance_factor << std::endl;
        dump(node->_right.get());
    }
    void dump() { std::cout << "root: " << root()->_value.first << std::endl; dump(root()); }
};

} // end namespace avl


#endif // AVL_TREE_H
