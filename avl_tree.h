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
    using node_val_type = std::pair<const key_type, val_type>;
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

        friend bool operator==(const Node &lhs, const Node &rhs) noexcept { return lhs._value == rhs._value; }
        friend bool operator!=(const Node &lhs, const Node &rhs) noexcept { return lhs._value != rhs._value; }
    };

    using node_type = Node;

    size_type _size{0};
    const cmp_type _comparator{};
    std::unique_ptr<node_type> _root_sentinel{nullptr};
    node_type *_begin{nullptr};

    node_type *root() noexcept { return _root_sentinel->_left.get(); }
    const node_type *root() const noexcept { return _root_sentinel->_left.get(); }

    static node_type *smallest_subtree_elt(node_type *root) noexcept {
        if (root->_left == nullptr)
            return root;
        return smallest_subtree_elt(root->_left.get());
    }

    static node_type *largest_subtree_elt(node_type *root) noexcept {
        if (root->_right == nullptr)
            return root;
        return largest_subtree_elt(root->_right.get());
    }

    template <class ValT>
    node_type *insert_internal(node_type *insert, ValT &&value) {
        assert(insert && "Inserting at nullptr.");

        if (!_comparator(value.first, insert->_value.first) && !_comparator(insert->_value.first, value.first))
            return insert;
        else if (_comparator(value.first, insert->_value.first)) {
            if (insert->_left == nullptr) {
                insert->_left = std::make_unique<node_type>(std::forward<ValT>(value), insert);
                if (insert == _begin)
                    _begin = insert->_left.get();
                ++_size;
                return insert->_left.get();
            } else
                return insert_internal(insert->_left.get(), std::forward<ValT>(value));
        } else {
            if (insert->_right == nullptr) {
                insert->_right = std::make_unique<node_type>(std::forward<ValT>(value), insert);
                ++_size;
                return insert->_right.get();
            } else {
                return insert_internal(insert->_right.get(), std::forward<ValT>(value));
            }
        }
    }

    void erase_internal(node_type *pos) noexcept {
        auto single_child = [pos, this]() -> std::unique_ptr<node_type>& {
            if (pos->_left && !pos->_right)
                return pos->_left;
            else if (pos->_right && !pos->_left)
                return pos->_right;
            else
                return _root_sentinel;
        };

        // Leaf node.
        if (!pos->_left && !pos->_right) {
            auto *parent = pos->_parent;
            if (pos == parent->_left.get()) {
                parent->_left.reset(nullptr);
                if (pos == _begin)
                    _begin = parent;
            } else
                parent->_right.reset(nullptr);
        // Node has a single child.
        } else if (auto &child = single_child(); child.get() != _root_sentinel.get()) {
            auto *pos_parent = pos->_parent;
            if (pos == pos->_parent->_left.get()) {
                std::swap(pos_parent->_left, child);
                pos_parent->_left->_parent = pos_parent;
                if (pos == _begin)
                    _begin = pos_parent->_left.get();
                child.reset(nullptr);
            } else {
                std::swap(pos_parent->_right, child);
                pos_parent->_right->_parent = pos_parent;
                child.reset(nullptr);
            }
        // Node has two children.
        } else {
            auto *swap_node = smallest_subtree_elt(pos->_right.get());
            auto *swap_node_parent = swap_node->_parent;
            auto *pos_parent = pos->_parent;
            std::swap(get_unique_ptr(pos), get_unique_ptr(swap_node));
            std::swap(swap_node->_left, pos->_left);
            std::swap(swap_node->_right, pos->_right);

            swap_node->_parent = pos_parent;
            pos->_parent = swap_node_parent;
            if (pos->_right)
                pos->_right->_parent = pos;
            swap_node->_left->_parent = swap_node;
            swap_node->_right->_parent = swap_node;
            return erase_internal(pos);
        }
    }

    node_type *find_internal(node_type *root, const key_type& key) const noexcept {
        if (!root)
            return _root_sentinel.get();
        else if (!_comparator(root->_value.first, key) && !_comparator(key, root->_value.first))
            return root;
        else if (_comparator(key, root->_value.first))
            return find_internal(root->_left.get(), key);
        else
            return find_internal(root->_right.get(), key);
    }

    val_type &at_internal(const key_type &key) {
        if (auto *node = find_internal(root(), key); node != _root_sentinel.get())
            return node->_value.second;

        throw std::out_of_range("Nonexistent key.\n");
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
        if (new_root->_balance_factor == 0) {
            new_root->_balance_factor = -1;
            new_root->_left->_balance_factor = 1;
        } else {
            new_root->_balance_factor = 0;
            new_root->_left->_balance_factor = 0;
        }
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
        if (new_root->_balance_factor == 0) {
            new_root->_balance_factor = 1;
            new_root->_right->_balance_factor = -1;
        } else {
            new_root->_balance_factor = 0;
            new_root->_right->_balance_factor = 0;
        }
    }

    void rotate_subtree_right_left(std::unique_ptr<node_type> &old_root) noexcept {
        auto *old_root_parent = old_root->_parent;
        auto *child = old_root->_right.get();
        auto *new_root = old_root->_right->_left.get();
        std::swap(new_root->_right, child->_left);
        std::swap(old_root->_right, new_root->_right);
        new_root->_parent = old_root.get();
        new_root->_right->_parent = new_root;
        if (new_root->_right->_left)
            new_root->_right->_left->_parent = new_root->_right.get();

        std::swap(new_root->_left, old_root->_right);
        std::swap(old_root, new_root->_left);
        new_root->_parent = old_root_parent;
        new_root->_left->_parent = new_root;
        if (new_root->_left->_right)
            new_root->_left->_right->_parent = new_root->_left.get();

        if (new_root->_balance_factor == 0) {
            new_root->_left->_balance_factor = 0;
            new_root->_right->_balance_factor = 0;
        } else if (new_root->_balance_factor > 0) {
            new_root->_left->_balance_factor = -1;
            new_root->_right->_balance_factor = 0;
        } else {
            assert(new_root->_balance_factor == -1);
            new_root->_left->_balance_factor = 0;
            new_root->_right->_balance_factor = 1;
        }
        new_root->_balance_factor = 0;
    }

    void rotate_subtree_left_right(std::unique_ptr<node_type> &old_root) noexcept {
        auto *old_root_parent = old_root->_parent;
        auto *child = old_root->_left.get();
        auto *new_root = old_root->_left->_right.get();
        std::swap(new_root->_left, child->_right);
        std::swap(old_root->_left, new_root->_left);
        new_root->_parent = old_root.get();
        new_root->_left->_parent = new_root;
        if (new_root->_left->_right)
            new_root->_left->_right->_parent = new_root->_left.get();

        std::swap(new_root->_right, old_root->_left);
        std::swap(old_root, new_root->_right);
        new_root->_parent = old_root_parent;
        new_root->_right->_parent = new_root;
        if (new_root->_right->_left)
            new_root->_right->_left->_parent = new_root->_right.get();

        if (new_root->_balance_factor == 0) {
            new_root->_left->_balance_factor = 0;
            new_root->_right->_balance_factor = 0;
        } else if (new_root->_balance_factor < 0) {
            new_root->_right->_balance_factor = 1;
            new_root->_left->_balance_factor = 0;
        } else {
            assert(new_root->_balance_factor == 1);
            new_root->_right->_balance_factor = 0;
            new_root->_left->_balance_factor = -1;
        }
        new_root->_balance_factor = 0;
    }

    void retrace_insert(std::unique_ptr<node_type> &node) {
        auto *parent = node->_parent;
        if (parent == _root_sentinel.get())
            return;

        [[maybe_unused]] const auto *left_child = parent->_left.get();
        const auto *right_child = parent->_right.get();
        if (node.get() == right_child) {
            if (parent->_balance_factor > 0) {
                if (node->_balance_factor >= 0)
                    rotate_subtree_left(get_unique_ptr(parent));
                else
                    rotate_subtree_right_left(get_unique_ptr(parent));
            } else if (parent->_balance_factor < 0) {
                parent->_balance_factor = 0;
            } else {
                ++parent->_balance_factor;
                return retrace_insert(get_unique_ptr(parent));
            }
        } else {
            assert(node.get() == left_child);
            if (parent->_balance_factor < 0) {
                if (node->_balance_factor <= 0)
                    rotate_subtree_right(get_unique_ptr(parent));
                else
                    rotate_subtree_left_right(get_unique_ptr(parent));
            } else if (parent->_balance_factor > 0) {
                parent->_balance_factor = 0;
            } else {
                --parent->_balance_factor;
                return retrace_insert(get_unique_ptr(parent));
            }
        }
    }

    void retrace_erase(std::unique_ptr<node_type> &node) {
        auto *parent = node->_parent;
        if (parent == _root_sentinel.get())
            return;

        const auto *left_child = parent->_left.get();
        [[maybe_unused]] const auto *right_child = parent->_right.get();
        if (node.get() == left_child) {
            if (parent->_balance_factor > 0) {
                if (right_child->_balance_factor < 0)
                    rotate_subtree_right_left(get_unique_ptr(parent));
                else
                    rotate_subtree_left(get_unique_ptr(parent));
            } else if (parent->_balance_factor == 0) {
                parent->_balance_factor = 1;
            } else {
                parent->_balance_factor = 0;
                return retrace_erase(get_unique_ptr(parent));
            }
        } else {
            assert(node.get() == right_child);
            if (parent->_balance_factor < 0) {
                if (left_child->_balance_factor > 0)
                    rotate_subtree_left_right(get_unique_ptr(parent));
                else
                    rotate_subtree_right(get_unique_ptr(parent));
            } else if (parent->_balance_factor == 0) {
                parent->_balance_factor = -1;
            } else {
                parent->_balance_factor = 0;
                return retrace_erase(get_unique_ptr(parent));
            }
        }
    }

public:
    template <typename ItT>
    struct Iterator final {
        friend class avl_tree<Key, T, Cmp>;

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

        friend bool operator==(const Iterator<ItT> &lhs, const Iterator<ItT> &rhs) noexcept { return lhs._ptr == rhs._ptr; }
        friend bool operator!=(const Iterator<ItT> &lhs, const Iterator<ItT> &rhs) noexcept { return lhs._ptr != rhs._ptr; }

    private:
        node_type *_ptr;

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

    template <class... Args>
    [[maybe_unused]] std::pair<iterator, bool> emplace(Args&&... args) {
        if (!root()) {
            _root_sentinel->_left = std::make_unique<node_type>(node_val_type(std::forward<Args>(args)...), _root_sentinel.get());
            _begin = root();
            ++_size;
            return std::make_pair(iterator(root()), true);
        }

        if (_size == max_size())
            return std::make_pair(end(), false);

        auto *new_node = insert_internal(root(), std::forward<Args>(args)...);
        retrace_insert(get_unique_ptr(new_node));

        return std::make_pair(iterator(new_node), true);
    }

    template <class... Args>
    [[maybe_unused]] std::pair<iterator, bool> try_emplace(const key_type &key, Args&&... args) {
        if (auto it = find(key); it != end())
            return std::make_pair(it, true);

        if (!root()) {
            _root_sentinel->_left = std::make_unique<node_type>(node_val_type(std::piecewise_construct, std::forward_as_tuple(key),
                                                                              std::forward_as_tuple(std::forward<Args>(args)...)),
                                                                _root_sentinel.get());
            _begin = root();
            ++_size;
            return std::make_pair(iterator(root()), true);
        }

        if (_size == max_size())
            return std::make_pair(end(), false);

        auto new_node_val = node_val_type(std::piecewise_construct, std::forward_as_tuple(key),
                                          std::forward_as_tuple(std::forward<Args>(args)...));
        auto *new_node = insert_internal(root(), std::move(new_node_val));
        retrace_insert(get_unique_ptr(new_node));

        return std::make_pair(iterator(new_node), true);
    }

    [[maybe_unused]] std::pair<iterator, bool> insert(const node_val_type &value) { return emplace(value); }
    [[maybe_unused]] std::pair<iterator, bool> insert(node_val_type &&value) { return emplace(std::move(value)); }
    [[maybe_unused]] std::pair<iterator, bool> insert(const val_type &value) { return emplace(std::make_pair(value, value)); }

    template <class InsT>
    [[maybe_unused]] std::pair<iterator, bool> insert(InsT &&value) { return emplace(std::forward<InsT>(value)); }

    template <class ItT>
    void insert(ItT first, ItT last) {
        for (; first != last; ++first)
            insert(*first);
    }

    void insert(std::initializer_list<node_val_type> ilist) {
        for (auto val : ilist)
            insert(val);
    }

    iterator erase(iterator pos) {
        if (pos == end())
            throw std::out_of_range("Invalid iterator.\n");
        auto ret_it = pos;
        ++ret_it;

        retrace_erase(get_unique_ptr(pos._ptr));
        erase_internal(pos._ptr);
        --_size;

        return ret_it;
    }

    iterator find(const key_type &key) { return iterator(find_internal(root(), key)); }
    const_iterator find(const key_type &key) const {return const_iterator(find_internal(root(), key));}

    bool friend operator==(const avl_tree<T, Key, Cmp> &lhs, const avl_tree<T, Key, Cmp> &rhs) noexcept {
        if (lhs.size() != rhs.size())
            return false;

        for (auto it_lhs = lhs.cbegin(), it_rhs = rhs.cbegin(); it_lhs != lhs.cend(); ++it_lhs, ++it_rhs) {
            if (*it_lhs != *it_rhs)
                return false;
        }

        return true;
    }
    bool friend operator!=(const avl_tree<T, Key, Cmp> &lhs, const avl_tree<T, Key, Cmp> &rhs) noexcept { return !(lhs == rhs); }
    bool friend operator<(const avl_tree<T, Key, Cmp> &lhs, const avl_tree<T, Key, Cmp> &rhs) noexcept {
        auto it_lhs = lhs.cbegin();
        auto it_rhs = rhs.cbegin();
        for (; it_lhs != lhs.cend() || it_rhs != rhs.cend(); ++it_lhs, ++it_rhs) {
            if (*it_lhs != *it_rhs)
                return *it_lhs < *it_rhs;
        }

        if (it_lhs == lhs.cend() && it_rhs != rhs.cend())
            return true;

        return false;
    }
    bool friend operator<=(const avl_tree<T, Key, Cmp> &lhs, const avl_tree<T, Key, Cmp> &rhs) noexcept { return (lhs < rhs) || (lhs == rhs); }
    bool friend operator>(const avl_tree<T, Key, Cmp> &lhs, const avl_tree<T, Key, Cmp> &rhs) noexcept { return !(lhs <= rhs); }
    bool friend operator>=(const avl_tree<T, Key, Cmp> &lhs, const avl_tree<T, Key, Cmp> &rhs) noexcept { return !(lhs < rhs); }

    val_type &at(const key_type &key) { return at_internal(key); }
    const val_type &at(const key_type &key) const { return at_internal(key); }
    val_type &operator[](const key_type &key) noexcept {
        if (auto *node = find_internal(root(), key); node != _root_sentinel.get()) {
            return node->_value.second;
        }

        return emplace(std::make_pair(key, val_type())).first->second;
    }

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
