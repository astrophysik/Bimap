#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>

namespace bimap_details {
struct treap_base_node {
  uint32_t random_key = rand();
  treap_base_node* left{nullptr};
  treap_base_node* right{nullptr};
  treap_base_node* parent{nullptr};
};

template <typename T, typename Tag>
struct treap_node : treap_base_node {
  treap_node(const T& value) : key(value) {}
  treap_node(T&& value) : key(std::move(value)) {}

  T key;
};

template <typename T, typename Tag>
struct treap_head_node : treap_base_node {
  using node_ptr = treap_node<T, Tag>*;

  treap_head_node(node_ptr ptr) {
    left = ptr;
  }

  node_ptr get_root() {
    return static_cast<node_ptr>(left);
  }
};

template <typename T, typename U>
struct node : treap_node<T, struct Left_Tag>, treap_node<U, struct Right_Tag> {
  using left_base = treap_node<T, struct Left_Tag>;
  using right_base = treap_node<U, struct Right_Tag>;

  node(const T& first, const U& second)
      : left_base(first), right_base(second) {}
  node(T&& first, const U& second)
      : left_base(std::move(first)), right_base(second) {}
  node(const T& first, U&& second)
      : left_base(first), right_base(std::move(second)) {}
  node(T&& first, U&& second)
      : left_base(std::move(first)), right_base(std::move(second)) {}
};

template <typename Key, typename Comparator, typename Tag>
struct treap {
  using key_t = Key;
  using node_t = treap_node<Key, Tag>;
  using head_node_t = treap_head_node<Key, Tag>;

  struct iterator {
    friend struct treap<Key, Comparator, Tag>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using pointer = Key const*;
    using reference = Key const&;

    reference operator*() const {
      return static_cast<node_t*>(m_ptr)->key;
    }

    pointer operator->() const {
      return &static_cast<node_t*>(m_ptr)->key;
    }

    iterator& operator++() {
      if (!m_ptr->parent) {
        return *this;
      }
      if (m_ptr->right) {
        m_ptr = m_ptr->right;
        while (m_ptr->left) {
          m_ptr = m_ptr->left;
        }
      } else {
        auto temp = m_ptr->parent;
        while (temp && m_ptr == temp->right) {
          m_ptr = temp;
          temp = temp->parent;
        }
        if (m_ptr->right != temp) {
          m_ptr = temp;
        }
      }
      return *this;
    }

    iterator operator++(int) {
      iterator temp = *this;
      ++(*this);
      return temp;
    }

    iterator& operator--() {
      if (m_ptr->left) {
        m_ptr = m_ptr->left;
        while (m_ptr->right) {
          m_ptr = m_ptr->right;
        }
      } else {
        if (!m_ptr->parent) {
          m_ptr = nullptr;
          return *this;
        }
        auto temp = m_ptr->parent;
        while (temp && m_ptr == temp->left) {
          m_ptr = temp;
          temp = temp->parent;
        }
        if (m_ptr->left != temp) {
          m_ptr = temp;
        }
      }
      return *this;
    }

    iterator operator--(int) {
      iterator temp = *this;
      --(*this);
      return temp;
    }

    friend bool operator==(const iterator& lhs, const iterator& rhs) {
      return lhs.m_ptr == rhs.m_ptr;
    }

    friend bool operator!=(const iterator& lhs, const iterator& rhs) {
      return lhs.m_ptr != rhs.m_ptr;
    }

    node_t* get_node() const {
      return static_cast<node_t*>(m_ptr);
    }

    explicit iterator(treap_base_node* ptr) : m_ptr(ptr) {}

  private:
    treap_base_node* m_ptr;
  };

  treap(Comparator comp) : head_node(nullptr), comparator(std::move(comp)) {}

  treap(const treap& other)
      : head_node(nullptr), comparator(other.comparator) {}

  treap(treap&& other)
      : head_node(other.head_node), comparator(std::move(other.comparator)) {
    other.head_node.left = nullptr;
  }

  treap& operator=(const treap& other) noexcept {
    comparator = other.comparator;
    return *this;
  }

  treap& operator=(treap&& other) noexcept {
    head_node.left = other.head_node.left;
    comparator = std::move(other.comparator);
    other.head_node.left = nullptr;
    return *this;
  }

  std::pair<node_t*, node_t*> split(node_t* root, const key_t& key) const {
    if (root == nullptr) {
      return {nullptr, nullptr};
    } else if (comparator(root->key, key)) {
      auto [l, r] = split(static_cast<node_t*>(root->right), key);
      root->right = l;
      if (l)
        l->parent = root;
      if (r)
        r->parent = nullptr;
      return {root, r};
    } else {
      auto [l, r] = split(static_cast<node_t*>(root->left), key);
      root->left = r;
      if (r)
        r->parent = root;
      if (l)
        l->parent = nullptr;
      return {l, root};
    }
  }

  node_t* merge(node_t* t1, node_t* t2) const {
    if (t1 == nullptr) {
      if (t2)
        t2->parent = nullptr;
      return t2;
    } else if (t2 == nullptr) {
      if (t1)
        t1->parent = nullptr;
      return t1;
    } else if (t1->random_key > t2->random_key) {
      t1->right = merge(static_cast<node_t*>(t1->right), t2);
      if (t1->right)
        t1->right->parent = t1;
      return t1;
    } else {
      t2->left = merge(t1, static_cast<node_t*>(t2->left));
      if (t2->left)
        t2->left->parent = t2;
      return t2;
    }
  }

  iterator insert(node_t* node) {
    auto [left, right] = split(head_node.get_root(), node->key);
    auto new_left = merge(left, node);
    head_node.left = merge(new_left, right);
    head_node.left->parent = &head_node;
    return iterator(node);
  }

  iterator remove(const key_t& key) {
    auto [t1, t2] = split(head_node.get_root(), key);
    auto removed_node = t2;
    auto next = removed_node;
    while (removed_node->left) {
      removed_node = static_cast<node_t*>(removed_node->left);
    }
    if (comparator(removed_node->key, key) ||
        comparator(key, removed_node->key)) {
      return iterator(&head_node);
    }
    if (removed_node == t2) {
      t2 = static_cast<node_t*>(removed_node->right);
      next = t2;
    } else {
      removed_node->parent->left = removed_node->right;
      next = static_cast<node_t*>(removed_node->parent);
    }
    if (removed_node->right) {
      removed_node->right->parent = removed_node->parent;
      next = static_cast<node_t*>(removed_node->right);
    }
    head_node.left = merge(t1, t2);
    if (head_node.left)
      head_node.left->parent = &head_node;
    if (next == nullptr) {
      return iterator(&head_node);
    }
    return iterator(next);
  }

  iterator exist(const key_t& key) const {
    auto buffer = head_node.get_root();
    while (buffer != nullptr) {
      if (comparator(buffer->key, key)) {
        buffer = static_cast<node_t*>(buffer->right);
      } else if (comparator(key, buffer->key)) {
        buffer = static_cast<node_t*>(buffer->left);
      } else {
        return iterator(buffer);
      }
    }
    return iterator(&head_node);
  }

  iterator lower_bound(const key_t& key) const {
    auto [t1, t2] = split(head_node.get_root(), key);
    auto lower_b = t2;
    while (lower_b && lower_b->left) {
      lower_b = static_cast<node_t*>(lower_b->left);
    }
    head_node.left = merge(t1, t2);
    if (head_node.left)
      head_node.left->parent = &head_node;
    if (!lower_b) {
      return iterator(&head_node);
    }
    return iterator(lower_b);
  }

  iterator upper_bound(const key_t& key) const {
    auto [t1, t2] = split(head_node.get_root(), key);
    auto upper_b = t2;
    if (!upper_b) {
      head_node.left = merge(t1, t2);
      if (head_node.left)
        head_node.left->parent = &head_node;
      return iterator(&head_node);
    }
    while (upper_b && upper_b->left) {
      upper_b = static_cast<node_t*>(upper_b->left);
    }
    if (!comparator(upper_b->key, key) && !comparator(key, upper_b->key)) {
      if (upper_b->parent) {
        upper_b = static_cast<node_t*>(upper_b->parent);
      } else if (upper_b->right) {
        upper_b = static_cast<node_t*>(upper_b->right);
      }
    }
    head_node.left = merge(t1, t2);
    if (head_node.left)
      head_node.left->parent = &head_node;
    if (!upper_b) {
      return iterator(&head_node);
    }
    return iterator(upper_b);
  }

  iterator begin() const {
    if (!head_node.left) {
      return iterator(&head_node);
    }
    auto min = head_node.left;
    while (min->left) {
      min = min->left;
    }
    return iterator(min);
  }

  iterator end() const {
    return iterator(&head_node);
  }

  void swap(treap& other) noexcept {
    std::swap(comparator, other.comparator);
    auto root = head_node.left;
    head_node.left = other.head_node.left;
    if (head_node.left)
      head_node.left->parent = &head_node;
    other.head_node.left = root;
    if (other.head_node.left)
      other.head_node.left->parent = &other.head_node;
  }

private:
  mutable head_node_t head_node;
  [[no_unique_adress]] Comparator comparator;
};
} // namespace bimap_details