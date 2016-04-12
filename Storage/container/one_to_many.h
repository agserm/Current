/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2016 Grigory Nikolaenko <nikolaenko.grigory@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef CURRENT_STORAGE_CONTAINER_ONE_TO_MANY_H
#define CURRENT_STORAGE_CONTAINER_ONE_TO_MANY_H

#include "common.h"
#include "sfinae.h"

#include "../base.h"

#include "../../TypeSystem/optional.h"
#include "../../Bricks/util/comparators.h"  // For `CurrentHashFunction`.

namespace current {
namespace storage {
namespace container {

template <typename T,
          typename T_UPDATE_EVENT,
          typename T_DELETE_EVENT,
          template <typename...> class ROW_MAP,
          template <typename...> class COL_MAP>
class GenericOneToMany {
 public:
  using T_ROW = sfinae::ENTRY_ROW_TYPE<T>;
  using T_COL = sfinae::ENTRY_COL_TYPE<T>;
  using T_KEY = std::pair<T_ROW, T_COL>;
  using T_ELEMENTS_MAP = std::unordered_map<T_KEY, std::unique_ptr<T>, CurrentHashFunction<T_KEY>>;
  using T_FORWARD_MAP = ROW_MAP<T_ROW, COL_MAP<T_COL, const T*>>;
  using T_TRANSPOSED_MAP = COL_MAP<T_COL, const T*>;
  using T_REST_BEHAVIOR = rest::behavior::Matrix;

  explicit GenericOneToMany(MutationJournal& journal) : journal_(journal) {}

  bool Empty() const { return map_.empty(); }
  size_t Size() const { return map_.size(); }

  // Adds specified object and overwrites existing one if it has the same row and col.
  // Removes all other existing objects with the same col.
  void Add(const T& object) {
    const auto row = sfinae::GetRow(object);
    const auto col = sfinae::GetCol(object);
    const auto key = std::make_pair(row, col);
    const auto it = map_.find(key);
    if (it != map_.end()) {
      const T& previous_object = *(it->second);
      journal_.LogMutation(T_UPDATE_EVENT(object),
                           [this, key, previous_object]() { DoAdd(key, previous_object); });
    } else {
      const auto it = transposed_.find(col);
      if (it != transposed_.end()) {
        const T& previous_object = *(it->second);
        const auto previous_key = std::make_pair(sfinae::GetRow(previous_object), col);
        journal_.LogMutation(T_DELETE_EVENT(previous_object),
                             [this, previous_key, previous_object]() { DoAdd(previous_key, previous_object); });
        DoErase(previous_key);
      }
      journal_.LogMutation(T_UPDATE_EVENT(object), [this, key]() { DoErase(key); });
    }
    DoAdd(key, object);
  }

  void Erase(const T_KEY& key) {
    const auto it = map_.find(key);
    if (it != map_.end()) {
      const T& previous_object = *(it->second);
      journal_.LogMutation(T_DELETE_EVENT(previous_object),
                           [this, key, previous_object]() { DoAdd(key, previous_object); });
      DoErase(key);
    }
  }
  void Erase(sfinae::CF<T_ROW> row, sfinae::CF<T_COL> col) { Erase(std::make_pair(row, col)); }

  void EraseCol(sfinae::CF<T_COL> col) {
    const auto it = transposed_.find(col);
    if (it != transposed_.end()) {
      const T previous_object = *(it->second);
      const auto key = std::make_pair(sfinae::GetRow(previous_object), col);
      journal_.LogMutation(T_DELETE_EVENT(previous_object),
                           [this, key, previous_object]() { DoAdd(key, previous_object); });
      DoErase(key);
    }
  }

  ImmutableOptional<T> operator[](const T_KEY& key) const {
    const auto it = map_.find(key);
    if (it != map_.end()) {
      return ImmutableOptional<T>(FromBarePointer(), it->second.get());
    } else {
      return nullptr;
    }
  }
  ImmutableOptional<T> Get(sfinae::CF<T_ROW> row, sfinae::CF<T_COL> col) const {
    return operator[](std::make_pair(row, col));
  }
  ImmutableOptional<T> GetEntryFromCol(sfinae::CF<T_COL> col) const {
    const auto it = transposed_.find(col);
    if (it != transposed_.end()) {
      return ImmutableOptional<T>(FromBarePointer(), it->second);
    } else {
      return nullptr;
    }
  }

  bool DoesNotConflict(const T_KEY& key) const { return transposed_.find(key.second) == transposed_.end(); }
  bool DoesNotConflict(sfinae::CF<T_ROW> row, sfinae::CF<T_COL> col) const {
    return DoesNotConflict(std::make_pair(row, col));
  }

  void operator()(const T_UPDATE_EVENT& e) {
    const auto row = sfinae::GetRow(e.data);
    const auto col = sfinae::GetCol(e.data);
    DoAdd(std::make_pair(row, col), e.data);
  }
  void operator()(const T_DELETE_EVENT& e) { DoErase(std::make_pair(e.key.first, e.key.second)); }

  template <typename T_MAP>
  struct Iterator final {
    using T_ITERATOR = typename T_MAP::const_iterator;
    using T_KEY = typename T_MAP::key_type;
    T_ITERATOR iterator_;
    explicit Iterator(T_ITERATOR iterator) : iterator_(iterator) {}
    void operator++() { ++iterator_; }
    bool operator==(const Iterator& rhs) const { return iterator_ == rhs.iterator_; }
    bool operator!=(const Iterator& rhs) const { return !operator==(rhs); }
    sfinae::CF<T_KEY> key() const { return iterator_->first; }
    const T& operator*() const { return *iterator_->second; }
    const T* operator->() const { return iterator_->second; }
  };

  template <typename INNER_MAP>
  struct InnerAccessor final {
    using INNER_KEY = typename INNER_MAP::key_type;
    const INNER_MAP& map_;

    InnerAccessor(const INNER_MAP& map) : map_(map) {}

    bool Empty() const { return map_.empty(); }
    size_t Size() const { return map_.size(); }

    bool Has(const INNER_KEY& x) const { return map_.find(x) != map_.end(); }

    Iterator<INNER_MAP> begin() const { return Iterator<INNER_MAP>(map_.cbegin()); }
    Iterator<INNER_MAP> end() const { return Iterator<INNER_MAP>(map_.cend()); }
  };

  template <typename OUTER_MAP>
  struct OuterAccessor final {
    using OUTER_KEY = typename OUTER_MAP::key_type;
    using INNER_MAP = typename OUTER_MAP::mapped_type;
    const OUTER_MAP& map_;

    struct OuterIterator final {
      using T_ITERATOR = typename OUTER_MAP::const_iterator;
      T_ITERATOR iterator;
      explicit OuterIterator(T_ITERATOR iterator) : iterator(iterator) {}
      void operator++() { ++iterator; }
      bool operator==(const OuterIterator& rhs) const { return iterator == rhs.iterator; }
      bool operator!=(const OuterIterator& rhs) const { return !operator==(rhs); }
      sfinae::CF<OUTER_KEY> key() const { return iterator->first; }
      InnerAccessor<INNER_MAP> operator*() const { return InnerAccessor<INNER_MAP>(iterator->second); }
    };

    explicit OuterAccessor(const OUTER_MAP& map) : map_(map) {}

    bool Empty() const { return map_.empty(); }
    size_t Size() const { return map_.size(); }

    bool Has(const OUTER_KEY& x) const { return map_.find(x) != map_.end(); }

    ImmutableOptional<InnerAccessor<INNER_MAP>> operator[](OUTER_KEY key) const {
      const auto iterator = map_.find(key);
      if (iterator != map_.end()) {
        return std::move(std::make_unique<InnerAccessor<INNER_MAP>>(iterator->second));
      } else {
        return nullptr;
      }
    }

    OuterIterator begin() const { return OuterIterator(map_.cbegin()); }
    OuterIterator end() const { return OuterIterator(map_.cend()); }
  };

  const OuterAccessor<T_FORWARD_MAP> Rows() const { return OuterAccessor<T_FORWARD_MAP>(forward_); }

  const InnerAccessor<T_TRANSPOSED_MAP> Cols() const { return InnerAccessor<T_TRANSPOSED_MAP>(transposed_); }

  Iterator<T_ELEMENTS_MAP> begin() const { return Iterator<T_ELEMENTS_MAP>(map_.begin()); }
  Iterator<T_ELEMENTS_MAP> end() const { return Iterator<T_ELEMENTS_MAP>(map_.end()); }

 private:
  void DoErase(const T_KEY& key) {
    auto& map_row = forward_[key.first];
    map_row.erase(key.second);
    if (map_row.empty()) {
      forward_.erase(key.first);
    }
    transposed_.erase(key.second);
    map_.erase(key);
  }

  void DoAdd(const T_KEY& key, const T& object) {
    auto& placeholder = map_[key];
    placeholder = std::make_unique<T>(object);
    forward_[key.first][key.second] = placeholder.get();
    transposed_[key.second] = placeholder.get();
  }

  T_ELEMENTS_MAP map_;
  T_FORWARD_MAP forward_;
  T_TRANSPOSED_MAP transposed_;
  MutationJournal& journal_;
};

template <typename T, typename T_UPDATE_EVENT, typename T_DELETE_EVENT>
using UnorderedOneToMany = GenericOneToMany<T, T_UPDATE_EVENT, T_DELETE_EVENT, Unordered, Unordered>;

template <typename T, typename T_UPDATE_EVENT, typename T_DELETE_EVENT>
using OrderedOneToMany = GenericOneToMany<T, T_UPDATE_EVENT, T_DELETE_EVENT, Ordered, Ordered>;

}  // namespace container

template <typename T, typename E1, typename E2>  // Entry, update event, delete event.
struct StorageFieldTypeSelector<container::UnorderedOneToMany<T, E1, E2>> {
  static const char* HumanReadableName() { return "UnorderedOneToMany"; }
};

template <typename T, typename E1, typename E2>  // Entry, update event, delete event.
struct StorageFieldTypeSelector<container::OrderedOneToMany<T, E1, E2>> {
  static const char* HumanReadableName() { return "OrderedOneToMany"; }
};

}  // namespace storage
}  // namespace current

using current::storage::container::UnorderedOneToMany;
using current::storage::container::OrderedOneToMany;

#endif  // CURRENT_STORAGE_CONTAINER_ONE_TO_MANY_H
