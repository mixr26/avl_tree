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
    // Data structure representing a node in the tree. Holds the payload, balance factor, and pointers to
    // descendants and ancestor.
    struct Node final {
        // Payload is a <key, value> pair.
        node_val_type _value{};

        // Balance factor of the node determines which of its subtrees is taller ([-1,1] range allowed).
        balance_type _balance_factor{0};

        // Pointers to descendants and the ancestor. Parent pointer need not be unique_ptr as we don't
        // expect the children to outlive the parent (dark).
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

    // A "false" root used as the end() iterator. Its "_left" pointer points to the "real" root of the tree
    // (if the tree is not empty). Its position in the tree allows us to implement bidirectonal iterator
    // without a special case for the end() iterator.
    std::unique_ptr<node_type> _root_sentinel{nullptr};
    // Cached pointer to the first (bottom-left-most) node in the tree. Updated during the insertion and
    // deletion, if needed. Used for faster construction of the begin() iterator.
    node_type *_begin{nullptr};

    // Cached size of the tree (excluding the sentinel root).
    size_type _size{0};
    // Instance of the comparator class, used for key comparison.
    const cmp_type _comparator{};

    // Returns the "real" root of the tree.
    node_type *root() noexcept { return _root_sentinel->_left.get(); }
    const node_type *root() const noexcept { return _root_sentinel->_left.get(); }

    // Find the element with the smallest key in a subtree rooted at "node".
    static node_type *smallest_subtree_elt(node_type *node) noexcept {
        if (node->_left == nullptr)
            return node;
        return smallest_subtree_elt(node->_left.get());
    }

    // Find the element with the greatest key in a subtree rooted at "node".
    static node_type *greatest_subtree_elt(node_type *node) noexcept {
        if (node->_right == nullptr)
            return node;
        return largest_subtree_elt(node->_right.get());
    }

    // Creates a new node with the "value" payload and inserts it at the appropriate position in the tree.
    // The first call of this function should pass root node as "insert", while the subsequent recursive
    // calls will descend down the tree until the appropriate empty position is found.
    template <class ValT>
    node_type *insert_internal(node_type *insert, ValT &&value) {
        assert(insert && "Inserting at nullptr.");

        // If the element with the given key already exists, return it without inserting the new element.
        if (!_comparator(value.first, insert->_value.first) && !_comparator(insert->_value.first, value.first))
            return insert;
        // If the given key is less than the "insert" node's key, we will be inserting in the left subtree.
        else if (_comparator(value.first, insert->_value.first)) {
            if (insert->_left == nullptr) {
                insert->_left = std::make_unique<node_type>(std::forward<ValT>(value), insert);
                if (insert == _begin)
                    _begin = insert->_left.get();
                ++_size;
                return insert->_left.get();
            } else
                return insert_internal(insert->_left.get(), std::forward<ValT>(value));
        // Conversely, insert in the right subtree.
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

    // Erase the node at the given position.
    void erase_internal(node_type *pos) noexcept {
        auto single_child = [pos, this]() -> std::unique_ptr<node_type>& {
            if (pos->_left && !pos->_right)
                return pos->_left;
            else if (pos->_right && !pos->_left)
                return pos->_right;
            else
                return _root_sentinel;
        };

        // If the node at the given position is a leaf node, just reset the parent's
        // pointer.
        if (!pos->_left && !pos->_right) {
            auto *parent = pos->_parent;
            if (pos == parent->_left.get()) {
                parent->_left.reset(nullptr);
                if (pos == _begin)
                    _begin = parent;
            } else
                parent->_right.reset(nullptr);
        // If the node has a single child, swap it with the child and delete it.
        } else if (auto &child = single_child(); child.get() != _root_sentinel.get()) {
            auto *pos_parent = pos->_parent;
            if (pos == pos->_parent->_left.get()) {
                std::swap(pos_parent->_left, child);
                pos_parent->_left->_parent = pos_parent;
                if (pos == _begin)
                    _begin = pos_parent->_left.get();
                // The "child" pointer now points to the node we want to erase, but the
                // pointer itself is a member of that node. In order to delete the node,
                // we need to reset the pointer.
                child.reset(nullptr);
            } else {
                std::swap(pos_parent->_right, child);
                pos_parent->_right->_parent = pos_parent;
                // Same as above.
                child.reset(nullptr);
            }
        // If the node has two children, swap it with its successor (element with the smallest
        // key from the right subtree) and delete it.
        } else {
            // Swap the node with its successor, and fixup the parent pointers.
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
            // The node we want to erase now falls into one of the first two cases, so we
            // can erase it with a recursive call.
            return erase_internal(pos);
        }
    }

    // Recursively search the tree until we find the node with given key. If the key does
    // not exist in the tree, return the sentinel root - the end() iterator.
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

    // Bounds checking find - if the given key exists in the tree, return the reference
    // to the value tied to that key. If the key doesn't exist, throw an exception.
    val_type &at_internal(const key_type &key) {
        if (auto *node = find_internal(root(), key); node != _root_sentinel.get())
            return node->_value.second;

        throw std::out_of_range("Nonexistent key.\n");
    }

    // Helper function which returns the parent's unique_ptr pointing to the given node.
    std::unique_ptr<node_type> &get_unique_ptr(node_type *node) noexcept {
        auto *parent = node->_parent;
        assert(parent != nullptr);
        return node == parent->_left.get() ? parent->_left : parent->_right;
    }

    // "Retrace" and "rotate" functions are helper functions which make sure that the AVL
    // tree invariant (balance factor at each node is in range [-1, 1]) is satisfied after
    // each insertion/erasure.
    //
    // https://en.wikipedia.org/wiki/AVL_tree#Operations
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

    // Go up the tree after insertion and fixup the subtrees which have invalidated
    // the AVL tree invariant.
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

    // Go up the tree after erasure and fixup the subtrees which have invalidated
    // the AVL tree invariant.
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
    // Bidirectional iterator to the elements of the AVL tree.
    template <typename ItT>
    struct Iterator final {
        friend class avl_tree<Key, T, Cmp>;

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_cv_t<ItT>;
        using pointer = ItT *;
        using reference = ItT &;

        // The iterator is essentialy just a wrapper around the node pointer...
        Iterator(node_type *ptr) : _ptr(ptr) {}

        // ...but dereferencing the iterator gives us access to node's payload only.
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

        // Find the node with the next greater key in the tree.
        void next() noexcept {
            if (_ptr->_right)
                _ptr = smallest_subtree_elt(_ptr->_right.get());
            else {
                while (_ptr != _ptr->_parent->_left.get())
                    _ptr = _ptr->_parent;
                _ptr = _ptr->_parent;
            }
        }

        // Find the node with the next smaller key in the tree.
        void prev() noexcept {
            if (_ptr->_left)
                _ptr = greatest_subtree_elt(_ptr->_left.get());
            else {
                while (_ptr != _ptr->_parent->_right.get())
                    _ptr = _ptr->_parent;
                _ptr = _ptr->_parent;
            }
        }
    };

    // Export both const and non-cost iterator types to the user.
    using iterator = Iterator<node_val_type>;
    using const_iterator = Iterator<const node_val_type>;

    // (Constant) begin and end iterators.
    iterator begin() noexcept { return iterator(_begin); }
    const_iterator cbegin() const noexcept { return const_iterator(_begin); }
    iterator end() noexcept { return iterator(_root_sentinel.get()); }
    const_iterator cend() const noexcept { return const_iterator(_root_sentinel.get()); }

    // An empty constructor sets up the root sentinel and the begin pointer. Begin pointer points to
    // the root sentinel when the tree is empty, so that begin() and end() iterators are equal in that
    // case.
    avl_tree() : _root_sentinel{std::make_unique<node_type>()}, _begin{_root_sentinel.get()} {}

    // Is the tree empty.
    bool empty() const noexcept { return root() == nullptr; }
    // Return the size of the tree.
    bool size() const noexcept { return _size; }
    // Maximum number of elements in the tree.
    constexpr size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

    // Delete all the nodes in the tree. By resetting the root node, we set in motion the cascading erasure
    // of all the nodes pointed to by unique_ptrs.
    void clear() noexcept { _root_sentinel->_left.reset(nullptr); _size = 0; }

    // Move construct the node and insert it in the tree. Expects the argument to be a <key, value> pair reference.
    // Return the iterator to the newly inserted element.
    template <class... Args>
    [[maybe_unused]] std::pair<iterator, bool> emplace(Args&&... args) {
        // Special case of empty tree insertion.
        if (!root()) {
            _root_sentinel->_left = std::make_unique<node_type>(node_val_type(std::forward<Args>(args)...), _root_sentinel.get());
            _begin = root();
            ++_size;
            return std::make_pair(iterator(root()), true);
        }

        // Don't insert new nodes if the maximum size is reached.
        if (_size == max_size())
            return std::make_pair(end(), false);

        // Insert the node and fixup the tree to satisfy the AVL invariant.
        auto *new_node = insert_internal(root(), std::forward<Args>(args)...);
        retrace_insert(get_unique_ptr(new_node));

        return std::make_pair(iterator(new_node), true);
    }

    // Move construct the node and insert it in the tree. The first argument is the key, while consecutive arguments are used
    // to construct the value in the node. If the key exists, these arguments will not be moved from, and the iterator to
    // the existing element will be returned.
    template <class... Args>
    [[maybe_unused]] std::pair<iterator, bool> try_emplace(const key_type &key, Args&&... args) {
        if (auto it = find(key); it != end())
            return std::make_pair(it, true);

        // Special case of empty tree insertion.
        if (!root()) {
            _root_sentinel->_left = std::make_unique<node_type>(node_val_type(std::piecewise_construct, std::forward_as_tuple(key),
                                                                              std::forward_as_tuple(std::forward<Args>(args)...)),
                                                                _root_sentinel.get());
            _begin = root();
            ++_size;
            return std::make_pair(iterator(root()), true);
        }

        // Don't insert new nodes if the maximum size is reached.
        if (_size == max_size())
            return std::make_pair(end(), false);

        // Insert the node and fixup the tree to satisfy the AVL invariant.
        auto new_node_val = node_val_type(std::piecewise_construct, std::forward_as_tuple(key),
                                          std::forward_as_tuple(std::forward<Args>(args)...));
        auto *new_node = insert_internal(root(), std::move(new_node_val));
        retrace_insert(get_unique_ptr(new_node));

        return std::make_pair(iterator(new_node), true);
    }

    // Various "insert" function overloads.
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

    // Erase the node at "pos", and return the node that precedes it.
    iterator erase(iterator pos) {
        if (pos == end())
            throw std::out_of_range("Invalid iterator.\n");
        auto ret_it = pos;
        ++ret_it;

        // This case is a little different from insertion because we actually need the node not to be erased
        // before the retrace call. Luckily, we can just call retrace before the actual deletion, and act as
        // if the node had already been deleted.
        retrace_erase(get_unique_ptr(pos._ptr));
        erase_internal(pos._ptr);
        --_size;

        if (_size == 0)
            _begin = _root_sentinel.get();

        return ret_it;
    }

    // Return the iterator to the node with the given key if it exists, otherwise, return end().
    iterator find(const key_type &key) { return iterator(find_internal(root(), key)); }
    const_iterator find(const key_type &key) const {return const_iterator(find_internal(root(), key));}

    // Comparison operators.
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

    // Find the node with the given key. If such node exists, return a reference to its payload value. If it doesn't exist,
    // create a new node, and return the reference to its payload value.
    val_type &operator[](const key_type &key) noexcept {
        if (auto *node = find_internal(root(), key); node != _root_sentinel.get()) {
            return node->_value.second;
        }

        return emplace(std::make_pair(key, val_type())).first->second;
    }

    // Like "operator[]", but throws an exception if the node with the given key does not exist.
    val_type &at(const key_type &key) { return at_internal(key); }
    const val_type &at(const key_type &key) const { return at_internal(key); }
};

} // end namespace avl


#endif // AVL_TREE_H
