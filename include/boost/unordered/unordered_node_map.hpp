// Copyright (C) 2022 Christian Mazakas
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_UNORDERED_NODE_MAP_HPP_INCLUDED
#define BOOST_UNORDERED_UNORDERED_NODE_MAP_HPP_INCLUDED

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/unordered/detail/foa.hpp>
#include <boost/unordered/detail/type_traits.hpp>
#include <boost/unordered/unordered_node_map_fwd.hpp>

#include <boost/core/allocator_access.hpp>
#include <boost/functional/hash.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/throw_exception.hpp>

#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace boost {
  namespace unordered {

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4714) /* marked as __forceinline not inlined */
#endif

    namespace detail {
      template <typename Allocator> class ptr_allocator_adaptor
      {
        using alloc_traits = std::allocator_traits<Allocator>;
        using ptr_allocator = typename alloc_traits::template rebind_alloc<
          typename Allocator::value_type*>;
        using ptr_alloc_traits = std::allocator_traits<ptr_allocator>;

        template <class U> friend class ptr_allocator_adaptor;

        Allocator al;

      public:
        using pointer = typename boost::allocator_pointer<ptr_allocator>::type;
        using const_pointer =
          typename boost::allocator_const_pointer<ptr_allocator>::type;

        using void_pointer =
          typename boost::allocator_void_pointer<ptr_allocator>::type;

        using const_void_pointer =
          typename boost::allocator_const_void_pointer<ptr_allocator>::type;

        using value_type = typename Allocator::value_type*;
        using size_type =
          typename boost::allocator_size_type<ptr_allocator>::type;

        using difference_type =
          typename boost::allocator_difference_type<ptr_allocator>::type;

        template <typename U> struct rebind
        {
          using other = ptr_allocator_adaptor<
            typename alloc_traits::template rebind_alloc<U> >;
        };

        ptr_allocator_adaptor() = default;
        ptr_allocator_adaptor(const Allocator& al_) : al{al_} {}

        template <typename Allocator2>
        ptr_allocator_adaptor(
          const ptr_allocator_adaptor<Allocator2>& x) noexcept : al{x.al}
        {
        }

        template <typename Allocator2>
        bool operator==(
          const ptr_allocator_adaptor<Allocator2>& x) const noexcept
        {
          return al == x.al;
        }

        template <typename Allocator2>
        bool operator!=(
          const ptr_allocator_adaptor<Allocator2>& x) const noexcept
        {
          return al != x.al;
        }

        pointer allocate(std::size_t n)
        {
          ptr_allocator pal = al;
          return ptr_alloc_traits::allocate(pal, n);
        }

        void deallocate(pointer p, std::size_t n) noexcept
        {
          ptr_allocator pal = al;
          ptr_alloc_traits::deallocate(pal, p, n);
        }

        void construct(value_type* p, const value_type& x)
        {
          this->construct(p, *x);
        }

        void construct(value_type* p, value_type&& x)
        {
          *p = x;
          x = nullptr;
        }

        template <typename... Args>
        void construct(value_type* p, Args&&... args)
        {
          *p = boost::to_address(alloc_traits::allocate(al, 1));
          try {
            alloc_traits::construct(al, *p, std::forward<Args>(args)...);
          } catch (...) {
            alloc_traits::deallocate(al,
              boost::pointer_traits<typename alloc_traits::pointer>::pointer_to(
                **p),
              1);
            throw;
          }
        }

        void destroy(value_type* p) noexcept
        {
          if (*p) {
            alloc_traits::destroy(al, *p);
            alloc_traits::deallocate(al, *p, 1);
          }
        }
      };
    } // namespace detail

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    class unordered_node_map
    {
      struct map_types
      {
        using key_type = Key;
        using raw_key_type = typename std::remove_const<Key>::type;
        using raw_mapped_type = typename std::remove_const<T>::type;

        using init_type = std::pair<raw_key_type, raw_mapped_type>;
        using moved_type = std::pair<Key const, T>*;
        using value_type = std::pair<Key const, T>;

        using emplace_type = value_type;
        using storage_type = value_type*;

        template <class K, class V>
        static raw_key_type const& extract(std::pair<K, V> const& kv)
        {
          return kv.first;
        }

        template <class K, class V>
        static raw_key_type const& extract(std::pair<K, V> const* kv)
        {
          return kv->first;
        }

        static storage_type&& move(storage_type& x) { return std::move(x); }
      };

      using table_type = detail::foa::table<map_types, Hash, KeyEqual,
        detail::ptr_allocator_adaptor<typename boost::allocator_rebind<
          Allocator, std::pair<Key const, T> >::type> >;

      table_type table_;

      template <class K, class V, class H, class KE, class A, class Pred>
      typename unordered_node_map<K, V, H, KE, A>::size_type friend erase_if(
        unordered_node_map<K, V, H, KE, A>& set, Pred pred);

    public:
      using key_type = Key;
      using mapped_type = T;
      using value_type = std::pair<Key const, T>;
      using init_type = std::pair<typename map_types::raw_key_type,
        typename map_types::raw_mapped_type>;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using hasher = typename boost::type_identity<Hash>::type;
      using key_equal = typename boost::type_identity<KeyEqual>::type;
      using allocator_type = typename boost::type_identity<
        typename boost::allocator_rebind<Allocator, value_type>::type>::type;
      using reference = value_type&;
      using const_reference = value_type const&;
      using pointer = typename boost::allocator_pointer<allocator_type>::type;
      using const_pointer =
        typename boost::allocator_const_pointer<allocator_type>::type;
      using iterator = boost::indirect_iterator<typename table_type::iterator>;
      using const_iterator =
        boost::indirect_iterator<typename table_type::const_iterator>;

      unordered_node_map() : unordered_node_map(0) {}

      explicit unordered_node_map(size_type n, hasher const& h = hasher(),
        key_equal const& pred = key_equal(),
        allocator_type const& a = allocator_type())
          : table_(n, h, pred, detail::ptr_allocator_adaptor<allocator_type>(a))
      {
      }

      unordered_node_map(size_type n, allocator_type const& a)
          : unordered_node_map(n, hasher(), key_equal(), a)
      {
      }

      unordered_node_map(size_type n, hasher const& h, allocator_type const& a)
          : unordered_node_map(n, h, key_equal(), a)
      {
      }

      template <class InputIterator>
      unordered_node_map(
        InputIterator f, InputIterator l, allocator_type const& a)
          : unordered_node_map(f, l, size_type(0), hasher(), key_equal(), a)
      {
      }

      explicit unordered_node_map(allocator_type const& a)
          : unordered_node_map(0, a)
      {
      }

      template <class Iterator>
      unordered_node_map(Iterator first, Iterator last, size_type n = 0,
        hasher const& h = hasher(), key_equal const& pred = key_equal(),
        allocator_type const& a = allocator_type())
          : unordered_node_map(n, h, pred, a)
      {
        this->insert(first, last);
      }

      template <class Iterator>
      unordered_node_map(
        Iterator first, Iterator last, size_type n, allocator_type const& a)
          : unordered_node_map(first, last, n, hasher(), key_equal(), a)
      {
      }

      template <class Iterator>
      unordered_node_map(Iterator first, Iterator last, size_type n,
        hasher const& h, allocator_type const& a)
          : unordered_node_map(first, last, n, h, key_equal(), a)
      {
      }

      unordered_node_map(unordered_node_map const& other) : table_(other.table_)
      {
      }

      unordered_node_map(
        unordered_node_map const& other, allocator_type const& a)
          : table_(other.table_, a)
      {
      }

      unordered_node_map(unordered_node_map&& other)
        noexcept(std::is_nothrow_move_constructible<hasher>::value&&
            std::is_nothrow_move_constructible<key_equal>::value&&
              std::is_nothrow_move_constructible<allocator_type>::value)
          : table_(std::move(other.table_))
      {
      }

      unordered_node_map(unordered_node_map&& other, allocator_type const& al)
          : table_(std::move(other.table_), al)
      {
      }

      unordered_node_map(std::initializer_list<value_type> ilist,
        size_type n = 0, hasher const& h = hasher(),
        key_equal const& pred = key_equal(),
        allocator_type const& a = allocator_type())
          : unordered_node_map(ilist.begin(), ilist.end(), n, h, pred, a)
      {
      }

      unordered_node_map(
        std::initializer_list<value_type> il, allocator_type const& a)
          : unordered_node_map(il, size_type(0), hasher(), key_equal(), a)
      {
      }

      unordered_node_map(std::initializer_list<value_type> init, size_type n,
        allocator_type const& a)
          : unordered_node_map(init, n, hasher(), key_equal(), a)
      {
      }

      unordered_node_map(std::initializer_list<value_type> init, size_type n,
        hasher const& h, allocator_type const& a)
          : unordered_node_map(init, n, h, key_equal(), a)
      {
      }

      ~unordered_node_map() = default;

      unordered_node_map& operator=(unordered_node_map const& other)
      {
        table_ = other.table_;
        return *this;
      }

      unordered_node_map& operator=(unordered_node_map&& other) noexcept(
        noexcept(std::declval<table_type&>() = std::declval<table_type&&>()))
      {
        table_ = std::move(other.table_);
        return *this;
      }

      allocator_type get_allocator() const noexcept
      {
        return table_.get_allocator();
      }

      /// Iterators
      ///

      iterator begin() noexcept { return table_.begin(); }
      const_iterator begin() const noexcept { return table_.begin(); }
      const_iterator cbegin() const noexcept { return table_.cbegin(); }

      iterator end() noexcept { return table_.end(); }
      const_iterator end() const noexcept { return table_.end(); }
      const_iterator cend() const noexcept { return table_.cend(); }

      /// Capacity
      ///

      BOOST_ATTRIBUTE_NODISCARD bool empty() const noexcept
      {
        return table_.empty();
      }

      size_type size() const noexcept { return table_.size(); }

      size_type max_size() const noexcept { return table_.max_size(); }

      /// Modifiers
      ///

      void clear() noexcept { table_.clear(); }

      template <class Ty>
      BOOST_FORCEINLINE auto insert(Ty&& value)
        -> decltype(table_.insert(std::forward<Ty>(value)))
      {
        return table_.insert(std::forward<Ty>(value));
      }

      BOOST_FORCEINLINE std::pair<iterator, bool> insert(init_type&& value)
      {
        return table_.insert(std::move(value));
      }

      template <class Ty>
      BOOST_FORCEINLINE auto insert(const_iterator, Ty&& value)
        -> decltype(iterator(table_.insert(std::forward<Ty>(value)).first))
      {
        return table_.insert(std::forward<Ty>(value)).first;
      }

      BOOST_FORCEINLINE iterator insert(const_iterator, init_type&& value)
      {
        return table_
          .insert(std::move(value.first), std::move(value.second))
          .first;
      }

      template <class InputIterator>
      BOOST_FORCEINLINE void insert(InputIterator first, InputIterator last)
      {
        for (auto pos = first; pos != last; ++pos) {
          table_.emplace(*pos);
        }
      }

      void insert(std::initializer_list<value_type> ilist)
      {
        this->insert(ilist.begin(), ilist.end());
      }

      template <class M>
      std::pair<iterator, bool> insert_or_assign(key_type const& key, M&& obj)
      {
        auto iter_bool_pair = table_.try_emplace(key, std::forward<M>(obj));
        if (iter_bool_pair.second) {
          return iter_bool_pair;
        }
        (*(iter_bool_pair.first))->second = std::forward<M>(obj);
        return iter_bool_pair;
      }

      template <class M>
      std::pair<iterator, bool> insert_or_assign(key_type&& key, M&& obj)
      {
        auto iter_bool_pair =
          table_.try_emplace(std::move(key), std::forward<M>(obj));
        if (iter_bool_pair.second) {
          return iter_bool_pair;
        }
        (*(iter_bool_pair.first))->second = std::forward<M>(obj);
        return iter_bool_pair;
      }

      template <class M>
      iterator insert_or_assign(const_iterator, key_type const& key, M&& obj)
      {
        return this->insert_or_assign(key, std::forward<M>(obj)).first;
      }

      template <class M>
      iterator insert_or_assign(const_iterator, key_type&& key, M&& obj)
      {
        return this->insert_or_assign(std::move(key), std::forward<M>(obj))
          .first;
      }

      template <class... Args>
      BOOST_FORCEINLINE std::pair<iterator, bool> emplace(Args&&... args)
      {
        return table_.emplace(init_type(std::forward<Args>(args)...));
      }

      template <class... Args>
      BOOST_FORCEINLINE iterator emplace_hint(const_iterator, Args&&... args)
      {
        return table_.emplace(std::forward<Args>(args)...).first;
      }

      template <class... Args>
      BOOST_FORCEINLINE std::pair<iterator, bool> try_emplace(
        key_type const& key, Args&&... args)
      {
        return table_.try_emplace(key, std::forward<Args>(args)...);
      }

      template <class... Args>
      BOOST_FORCEINLINE std::pair<iterator, bool> try_emplace(
        key_type&& key, Args&&... args)
      {
        return table_.try_emplace(std::move(key), std::forward<Args>(args)...);
      }

      template <class... Args>
      BOOST_FORCEINLINE iterator try_emplace(
        const_iterator, key_type const& key, Args&&... args)
      {
        return table_.try_emplace(key, std::forward<Args>(args)...).first;
      }

      template <class... Args>
      BOOST_FORCEINLINE iterator try_emplace(
        const_iterator, key_type&& key, Args&&... args)
      {
        return table_.try_emplace(std::move(key), std::forward<Args>(args)...)
          .first;
      }

      BOOST_FORCEINLINE void erase(iterator pos) { table_.erase(pos); }
      BOOST_FORCEINLINE void erase(const_iterator pos)
      {
        return table_.erase(pos);
      }
      iterator erase(const_iterator first, const_iterator last)
      {
        while (first != last) {
          this->erase(first++);
        }
        return iterator{detail::foa::const_iterator_cast_tag{}, last};
      }

      BOOST_FORCEINLINE size_type erase(key_type const& key)
      {
        return table_.erase(key);
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        detail::transparent_non_iterable<K, unordered_node_map>::value,
        size_type>::type
      erase(K const& key)
      {
        return table_.erase(key);
      }

      void swap(unordered_node_map& rhs) noexcept(
        noexcept(std::declval<table_type&>().swap(std::declval<table_type&>())))
      {
        table_.swap(rhs.table_);
      }

      template <class H2, class P2>
      void merge(
        unordered_node_map<key_type, mapped_type, H2, P2, allocator_type>&
          source)
      {
        table_.merge(source.table_);
      }

      template <class H2, class P2>
      void merge(
        unordered_node_map<key_type, mapped_type, H2, P2, allocator_type>&&
          source)
      {
        table_.merge(std::move(source.table_));
      }

      /// Lookup
      ///

      mapped_type& at(key_type const& key)
      {
        auto pos = table_.find(key);
        if (pos != table_.end()) {
          return pos->second;
        }
        // TODO: someday refactor this to conditionally serialize the key and
        // include it in the error message
        //
        boost::throw_exception(
          std::out_of_range("key was not found in unordered_node_map"));
      }

      mapped_type const& at(key_type const& key) const
      {
        auto pos = table_.find(key);
        if (pos != table_.end()) {
          return pos->second;
        }
        boost::throw_exception(
          std::out_of_range("key was not found in unordered_node_map"));
      }

      BOOST_FORCEINLINE mapped_type& operator[](key_type const& key)
      {
        return (*(table_.try_emplace(key).first))->second;
      }

      BOOST_FORCEINLINE mapped_type& operator[](key_type&& key)
      {
        return (*(table_.try_emplace(std::move(key)).first))->second;
      }

      BOOST_FORCEINLINE size_type count(key_type const& key) const
      {
        auto pos = table_.find(key);
        return pos != table_.end() ? 1 : 0;
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        detail::are_transparent<K, hasher, key_equal>::value, size_type>::type
      count(K const& key) const
      {
        auto pos = table_.find(key);
        return pos != table_.end() ? 1 : 0;
      }

      BOOST_FORCEINLINE iterator find(key_type const& key)
      {
        return table_.find(key);
      }

      BOOST_FORCEINLINE const_iterator find(key_type const& key) const
      {
        return table_.find(key);
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        boost::unordered::detail::are_transparent<K, hasher, key_equal>::value,
        iterator>::type
      find(K const& key)
      {
        return table_.find(key);
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        boost::unordered::detail::are_transparent<K, hasher, key_equal>::value,
        const_iterator>::type
      find(K const& key) const
      {
        return table_.find(key);
      }

      BOOST_FORCEINLINE bool contains(key_type const& key) const
      {
        return this->find(key) != this->end();
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        boost::unordered::detail::are_transparent<K, hasher, key_equal>::value,
        bool>::type
      contains(K const& key) const
      {
        return this->find(key) != this->end();
      }

      std::pair<iterator, iterator> equal_range(key_type const& key)
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      std::pair<const_iterator, const_iterator> equal_range(
        key_type const& key) const
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      template <class K>
      typename std::enable_if<
        detail::are_transparent<K, hasher, key_equal>::value,
        std::pair<iterator, iterator> >::type
      equal_range(K const& key)
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      template <class K>
      typename std::enable_if<
        detail::are_transparent<K, hasher, key_equal>::value,
        std::pair<const_iterator, const_iterator> >::type
      equal_range(K const& key) const
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      /// Hash Policy
      ///

      size_type bucket_count() const noexcept { return table_.capacity(); }

      float load_factor() const noexcept { return table_.load_factor(); }

      float max_load_factor() const noexcept
      {
        return table_.max_load_factor();
      }

      void max_load_factor(float) {}

      size_type max_load() const noexcept { return table_.max_load(); }

      void rehash(size_type n) { table_.rehash(n); }

      void reserve(size_type n) { table_.reserve(n); }

      /// Observers
      ///

      hasher hash_function() const { return table_.hash_function(); }

      key_equal key_eq() const { return table_.key_eq(); }
    };

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    bool operator==(
      unordered_node_map<Key, T, Hash, KeyEqual, Allocator> const& lhs,
      unordered_node_map<Key, T, Hash, KeyEqual, Allocator> const& rhs)
    {
      if (&lhs == &rhs) {
        return true;
      }

      return (lhs.size() == rhs.size()) && ([&] {
        for (auto const& kvp : lhs) {
          auto pos = rhs.find(kvp.first);
          if ((pos == rhs.end()) || (*pos != kvp)) {
            return false;
          }
        }
        return true;
      })();
    }

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    bool operator!=(
      unordered_node_map<Key, T, Hash, KeyEqual, Allocator> const& lhs,
      unordered_node_map<Key, T, Hash, KeyEqual, Allocator> const& rhs)
    {
      return !(lhs == rhs);
    }

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    void swap(unordered_node_map<Key, T, Hash, KeyEqual, Allocator>& lhs,
      unordered_node_map<Key, T, Hash, KeyEqual, Allocator>& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    template <class Key, class T, class Hash, class KeyEqual, class Allocator,
      class Pred>
    typename unordered_node_map<Key, T, Hash, KeyEqual, Allocator>::size_type
    erase_if(
      unordered_node_map<Key, T, Hash, KeyEqual, Allocator>& map, Pred pred)
    {
      return erase_if(map.table_, pred);
    }

#if defined(BOOST_MSVC)
#pragma warning(pop) /* C4714 */
#endif

#if BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES

    namespace detail {
      template <typename T>
      using iter_key_t =
        typename std::iterator_traits<T>::value_type::first_type;
      template <typename T>
      using iter_val_t =
        typename std::iterator_traits<T>::value_type::second_type;
      template <typename T>
      using iter_to_alloc_t =
        typename std::pair<iter_key_t<T> const, iter_val_t<T> >;
    } // namespace detail

    template <class InputIterator,
      class Hash =
        boost::hash<boost::unordered::detail::iter_key_t<InputIterator> >,
      class Pred =
        std::equal_to<boost::unordered::detail::iter_key_t<InputIterator> >,
      class Allocator = std::allocator<
        boost::unordered::detail::iter_to_alloc_t<InputIterator> >,
      class = boost::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = boost::enable_if_t<detail::is_hash_v<Hash> >,
      class = boost::enable_if_t<detail::is_pred_v<Pred> >,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(InputIterator, InputIterator,
      std::size_t = boost::unordered::detail::foa::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      -> unordered_node_map<boost::unordered::detail::iter_key_t<InputIterator>,
        boost::unordered::detail::iter_val_t<InputIterator>, Hash, Pred,
        Allocator>;

    template <class Key, class T,
      class Hash = boost::hash<boost::remove_const_t<Key> >,
      class Pred = std::equal_to<boost::remove_const_t<Key> >,
      class Allocator = std::allocator<std::pair<const Key, T> >,
      class = boost::enable_if_t<detail::is_hash_v<Hash> >,
      class = boost::enable_if_t<detail::is_pred_v<Pred> >,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(std::initializer_list<std::pair<Key, T> >,
      std::size_t = boost::unordered::detail::foa::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      -> unordered_node_map<boost::remove_const_t<Key>, T, Hash, Pred,
        Allocator>;

    template <class InputIterator, class Allocator,
      class = boost::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(InputIterator, InputIterator, std::size_t, Allocator)
      -> unordered_node_map<boost::unordered::detail::iter_key_t<InputIterator>,
        boost::unordered::detail::iter_val_t<InputIterator>,
        boost::hash<boost::unordered::detail::iter_key_t<InputIterator> >,
        std::equal_to<boost::unordered::detail::iter_key_t<InputIterator> >,
        Allocator>;

    template <class InputIterator, class Allocator,
      class = boost::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(InputIterator, InputIterator, Allocator)
      -> unordered_node_map<boost::unordered::detail::iter_key_t<InputIterator>,
        boost::unordered::detail::iter_val_t<InputIterator>,
        boost::hash<boost::unordered::detail::iter_key_t<InputIterator> >,
        std::equal_to<boost::unordered::detail::iter_key_t<InputIterator> >,
        Allocator>;

    template <class InputIterator, class Hash, class Allocator,
      class = boost::enable_if_t<detail::is_hash_v<Hash> >,
      class = boost::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(
      InputIterator, InputIterator, std::size_t, Hash, Allocator)
      -> unordered_node_map<boost::unordered::detail::iter_key_t<InputIterator>,
        boost::unordered::detail::iter_val_t<InputIterator>, Hash,
        std::equal_to<boost::unordered::detail::iter_key_t<InputIterator> >,
        Allocator>;

    template <class Key, class T, class Allocator,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(std::initializer_list<std::pair<Key, T> >, std::size_t,
      Allocator) -> unordered_node_map<boost::remove_const_t<Key>, T,
      boost::hash<boost::remove_const_t<Key> >,
      std::equal_to<boost::remove_const_t<Key> >, Allocator>;

    template <class Key, class T, class Allocator,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(std::initializer_list<std::pair<Key, T> >, Allocator)
      -> unordered_node_map<boost::remove_const_t<Key>, T,
        boost::hash<boost::remove_const_t<Key> >,
        std::equal_to<boost::remove_const_t<Key> >, Allocator>;

    template <class Key, class T, class Hash, class Allocator,
      class = boost::enable_if_t<detail::is_hash_v<Hash> >,
      class = boost::enable_if_t<detail::is_allocator_v<Allocator> > >
    unordered_node_map(std::initializer_list<std::pair<Key, T> >, std::size_t,
      Hash, Allocator) -> unordered_node_map<boost::remove_const_t<Key>, T,
      Hash, std::equal_to<boost::remove_const_t<Key> >, Allocator>;
#endif

  } // namespace unordered
} // namespace boost

#endif
