#pragma once

#include "treap.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;
  using node_t = bimap_details::node<left_t, right_t>;
  using left_node = bimap_details::treap_node<left_t, bimap_details::Left_Tag>;
  using right_node =
      bimap_details::treap_node<right_t, bimap_details::Right_Tag>;

  template <typename T, typename ComparatorT, typename TagT, typename U,
            typename ComparatorU, typename TagU>
  struct iterator {
    friend struct bimap<Left, Right, CompareLeft, CompareRight>;

    using tree_it =
        typename bimap_details::treap<T, ComparatorT, TagT>::iterator;

    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T const*;
    using reference = T const&;

    reference operator*() const {
      return *it;
    }

    pointer operator->() const {
      return &*it;
    }

    iterator& operator++() {
      ++it;
      return *this;
    }

    iterator operator++(int) {
      iterator temp = *this;
      ++(*this);
      return temp;
    }

    iterator& operator--() {
      --it;
      return *this;
    }

    iterator operator--(int) {
      iterator temp = *this;
      --(*this);
      return temp;
    }

    iterator<U, ComparatorU, TagU, T, ComparatorT, TagT> flip() const {
      return iterator<U, ComparatorU, TagU, T, ComparatorT, TagT>(
          typename bimap_details::treap<U, ComparatorU, TagU>::iterator(
              static_cast<bimap_details::treap_node<U, TagU>*>(
                  static_cast<node_t*>(it.get_node()))));
    }

    friend bool operator==(const iterator& lhs, const iterator& rhs) {
      return lhs.it == rhs.it;
    }

    friend bool operator!=(const iterator& lhs, const iterator& rhs) {
      return lhs.it != rhs.it;
    }

  private:
    explicit iterator(tree_it it) : it(it) {}
    tree_it it;
  };

  using left_iterator =
      iterator<left_t, CompareLeft, bimap_details::Left_Tag, right_t,
               CompareRight, bimap_details::Right_Tag>;
  using right_iterator =
      iterator<right_t, CompareRight, bimap_details::Right_Tag, left_t,
               CompareLeft, bimap_details::Left_Tag>;

  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : left_treap(std::move(compare_left)),
        right_treap(std::move(compare_right)) {}

  bimap(bimap const& other)
      : left_treap(other.left_treap), right_treap(other.right_treap) {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      try {
        insert(*it, *it.flip());
      } catch (...) {
        clear();
        break;
      }
    }
  }

  bimap(bimap&& other) noexcept
      : _size(other.size()), left_treap(std::move(other.left_treap)),
        right_treap(std::move(other.right_treap)) {}

  bimap& operator=(bimap const& other) {
    if (this == &other || other.empty()) {
      return *this;
    }
    clear();
    left_treap = other.left_treap;
    right_treap = other.right_treap;
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      try {
        insert(*it, *it.flip());
      } catch (...) {
        clear();
        break;
      }
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this == &other || other.empty()) {
      return *this;
    }
    clear();
    _size = other.size();
    left_treap = std::move(other.left_treap);
    right_treap = std::move(other.right_treap);
    return *this;
  }

  ~bimap() {
    clear();
  }

  template <typename T = left_t, typename U = right_t>
  left_iterator insert(T&& left, U&& right) {
    if (!empty() && (find_left(std::forward<T>(left)) != end_left() ||
                     find_right(std::forward<U>(right)) != end_right())) {
      return end_left();
    }
    node_t* ptr(new node_t(std::forward<T>(left), std::forward<U>(right)));
    auto l_it = left_treap.insert(static_cast<left_node*>(ptr));
    right_treap.insert(static_cast<right_node*>(ptr));
    _size++;
    return left_iterator(l_it);
  }

  left_iterator erase_left(left_iterator it) {
    auto current_it = it++;
    left_treap.remove(*current_it);
    right_treap.remove(*current_it.flip());
    delete static_cast<node_t*>(current_it.it.get_node());
    _size--;
    return it;
  }

  bool erase_left(left_t const& left) {
    auto it = find_left(left);
    if (it == end_left()) {
      return false;
    }
    erase_left(it);
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    auto current_it = it++;
    right_treap.remove(*current_it);
    left_treap.remove(*current_it.flip());
    delete static_cast<node_t*>(current_it.it.get_node());
    _size--;
    return it;
  }

  bool erase_right(right_t const& right) {
    auto it = find_right(right);
    if (it == end_right()) {
      return false;
    }
    erase_right(it);
    return true;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      first = erase_left(first);
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      first = erase_right(first);
    }
    return last;
  }

  left_iterator find_left(left_t const& left) const {
    left_iterator it(left_treap.exist(left));
    return it;
  }

  right_iterator find_right(right_t const& right) const {
    right_iterator it(right_treap.exist(right));
    return it;
  }

  right_t const& at_left(left_t const& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("Where is no such key in map");
    }
    return *it.flip();
  }

  left_t const& at_right(right_t const& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("Where is no such key in map");
    }
    return *it.flip();
  }

  template <typename T = right_t,
            typename = std::enable_if_t<std::is_default_constructible_v<T>>>
  right_t const& at_left_or_default(left_t const& key) {
    if (auto it = find_left(key); it != end_left()) {
      return *it.flip();
    }
    right_t dflt{};
    auto it = find_right(dflt);
    if (it != end_right()) {
      erase_right(it);
    }
    return *(insert(key, std::move(dflt))).flip();
  }

  template <typename T = left_t,
            typename = std::enable_if_t<std::is_default_constructible_v<T>>>
  left_t const& at_right_or_default(right_t const& key) {
    if (auto it = find_right(key); it != end_right()) {
      return *it.flip();
    }
    left_t dflt{};
    auto it = find_left(dflt);
    if (it != end_left()) {
      erase_left(it);
    }
    return *(insert(std::move(dflt), key));
  }

  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_treap.lower_bound(left));
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(left_treap.upper_bound(left));
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_treap.lower_bound(right));
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(right_treap.upper_bound(right));
  }

  left_iterator begin_left() const {
    return left_iterator(left_treap.begin());
  }

  left_iterator end_left() const {
    return left_iterator(left_treap.end());
  }

  right_iterator begin_right() const {
    return right_iterator(right_treap.begin());
  }

  right_iterator end_right() const {
    return right_iterator(right_treap.end());
  }

  bool empty() const {
    return size() == 0;
  }

  std::size_t size() const {
    return _size;
  }

  void clear() {
    erase_left(begin_left(), end_left());
  }

  friend bool operator==(bimap const& left, bimap const& right) {
    if (left.size() != right.size()) {
      return false;
    }
    auto buffer_left = left.begin_left();
    auto buffer_right = right.begin_left();
    while (buffer_left != left.end_left() && buffer_right != right.end_left()) {
      if (*buffer_left == *buffer_right &&
          *buffer_left.flip() == *buffer_right.flip()) {
        ++buffer_left;
        ++buffer_right;
      } else {
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(bimap const& left, bimap const& right) {
    return !(left == right);
  }

  void swap(bimap& other) noexcept {
    left_treap.swap(other.left_treap);
    right_treap.swap(other.right_treap);
  }

private:
  size_t _size{0};
  bimap_details::treap<left_t, CompareLeft, bimap_details::Left_Tag> left_treap;
  bimap_details::treap<right_t, CompareRight, bimap_details::Right_Tag> right_treap;
};