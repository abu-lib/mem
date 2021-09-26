// Copyright 2021 Francois Chabot

// Licensed under the Apache License, Version 2.0 (the "License")
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ABU_MEM_REF_COUNT_H_INCLUDED
#define ABU_MEM_REF_COUNT_H_INCLUDED

#include <concepts>
#include <utility>

#include "abu/mem/check.h"

namespace abu::mem {

namespace details_ {
struct basic_shared_state {
  basic_shared_state() = default;
  basic_shared_state(const basic_shared_state&) = delete;
  basic_shared_state(basic_shared_state&&) = delete;
  basic_shared_state& operator=(const basic_shared_state&) = delete;
  basic_shared_state& operator=(basic_shared_state&&) = delete;

  virtual ~basic_shared_state() = default;

  long ref_count = 0;
  void* ptr = nullptr;
};

template <typename T>
struct owned_shared_state final : basic_shared_state {
  template <typename... Args>
  explicit owned_shared_state(Args&&... args)
      : obj(std::forward<Args>(args)...) {
    ptr = &obj;
  }

  owned_shared_state(const owned_shared_state&) = delete;
  owned_shared_state(owned_shared_state&&) = delete;
  owned_shared_state& operator=(const owned_shared_state&) = delete;
  owned_shared_state& operator=(owned_shared_state&&) = delete;
  ~owned_shared_state() override = default;

  T obj;
};

template <typename T>
struct referenced_shared_state final : basic_shared_state {
  explicit referenced_shared_state(T* init) {
    ptr = init;
  }
  referenced_shared_state(const referenced_shared_state&) = delete;
  referenced_shared_state(referenced_shared_state&&) = delete;
  referenced_shared_state& operator=(const referenced_shared_state&) = delete;
  referenced_shared_state& operator=(referenced_shared_state&&) = delete;

  ~referenced_shared_state() override {
    delete static_cast<T*>(ptr);
  }
};
}  // namespace details_

template <typename T>
struct ref_count_traits {
  static void* create_shared_state(T* ptr) noexcept {
    assume(ptr);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto shared_state = new details_::referenced_shared_state<T>(ptr);
    shared_state->ref_count = 1;
    return shared_state;
  }

  static void add_ref(void* shared_state) noexcept {
    assume(shared_state);
    auto bss = static_cast<details_::basic_shared_state*>(shared_state);

    bss->ref_count += 1;
  }

  static void remove_ref(void* shared_state) noexcept {
    assume(shared_state);
    auto bss = static_cast<details_::basic_shared_state*>(shared_state);

    assume(bss->ref_count > 0);
    bss->ref_count -= 1;

    if (bss->ref_count == 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      delete bss;
    }
  }

  static long use_count(void* shared_state) noexcept {
    assume(shared_state);
    auto bss = static_cast<details_::basic_shared_state*>(shared_state);

    return bss->ref_count;
  }

  static T* resolve(void* shared_state) noexcept {
    assume(shared_state);
    auto bss = static_cast<details_::basic_shared_state*>(shared_state);

    return static_cast<T*>(bss->ptr);
  }

  template <typename... Args>
  static void* make_obj_and_shared_state(Args&&... args) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto shared_state =
        new details_::owned_shared_state<T>(std::forward<Args>(args)...);
    shared_state->ref_count = 1;
    return shared_state;
  }
};

class ref_counted {
  template <typename T>
  friend struct ref_count_traits;

  long ref_count_ = 0;

 protected:
  // Not virtual on purpose.
  ~ref_counted() = default;

  ref_counted() = default;
  ref_counted(const ref_counted&) = default;
  ref_counted(ref_counted&&) = default;
  ref_counted& operator=(const ref_counted&) = default;
  ref_counted& operator=(ref_counted&&) = default;
};

template <std::derived_from<ref_counted> T>
struct ref_count_traits<T> {
  static ref_counted* create_shared_state(T* ptr) noexcept {
    assume(ptr);
    ptr->ref_count_ += 1;
    return ptr;
  }

  static void add_ref(void* shared_state) noexcept {
    assume(shared_state);
    auto rc = static_cast<ref_counted*>(shared_state);

    rc->ref_count_ += 1;
  }

  static void remove_ref(void* shared_state) noexcept {
    assume(shared_state);
    auto rc = static_cast<ref_counted*>(shared_state);

    assume(rc->ref_count_ > 0);

    rc->ref_count_ -= 1;
    if (rc->ref_count_ == 0) {
      auto obj = static_cast<T*>(rc);
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      delete obj;
    }
  }

  static long use_count(void* shared_state) noexcept {
    assume(shared_state);
    auto rc = static_cast<ref_counted*>(shared_state);

    return rc->ref_count_;
  }

  static T* resolve(void* shared_state) noexcept {
    auto rc = static_cast<ref_counted*>(shared_state);
    return static_cast<T*>(rc);
  }

  template <typename... Args>

  static void* make_obj_and_shared_state(Args&&... args) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto result = new T(std::forward<Args>(args)...);
    result->ref_count_ = 1;
    return result;
  }
};

template <typename T>
class ref_count_ptr {
 public:
  using element_type = T;

  // ********** Constructors **********
  ref_count_ptr() noexcept = default;
  ref_count_ptr(std::nullptr_t) noexcept {}

  explicit ref_count_ptr(T* ptr) noexcept {
    if (ptr) {
      shared_state_ = ref_count_traits<T>::create_shared_state(ptr);
    }
  }

  template <std::derived_from<T> Y>
  explicit ref_count_ptr(Y* ptr) noexcept {
    if (ptr) {
      shared_state_ = ref_count_traits<T>::create_shared_state(ptr);
    }
  }

  ref_count_ptr(const ref_count_ptr& other) noexcept
      : shared_state_(other.shared_state_) {
    add_ref_();
  }

  template <std::derived_from<T> Y>
  ref_count_ptr(const ref_count_ptr<Y>& other) noexcept
      : shared_state_(other.shared_state_) {
    add_ref_();
  }

  ref_count_ptr(ref_count_ptr&& other) noexcept
      : shared_state_(other.shared_state_) {
    other.clear_();
  }

  template <std::derived_from<T> Y>
  ref_count_ptr(ref_count_ptr<Y>&& other) noexcept
      : shared_state_(other.shared_state_) {
    other.clear_();
  }

  // ********** Destructor **********
  ~ref_count_ptr() {
    release_();
  }

  // ********** operator=() **********
  ref_count_ptr& operator=(const ref_count_ptr& rhs) noexcept {
    release_();
    shared_state_ = rhs.shared_state_;
    add_ref_();
    return *this;
  }

  template <std::derived_from<T> Y>
  ref_count_ptr& operator=(const ref_count_ptr<Y>& rhs) noexcept {
    release_();
    shared_state_ = rhs.shared_state_;
    add_ref_();
    return *this;
  }

  ref_count_ptr& operator=(ref_count_ptr&& rhs) noexcept {
    release_();
    shared_state_ = rhs.shared_state_;
    rhs.clear_();
    return *this;
  }

  template <std::derived_from<T> Y>
  ref_count_ptr& operator=(ref_count_ptr<Y>&& rhs) noexcept {
    release_();
    shared_state_ = rhs.shared_state_;
    rhs.clear_();
    return *this;
  }

  // ********** reset() **********
  void reset() noexcept {
    release_();
    clear_();
  }

  // ********** get() **********
  element_type* get() const noexcept {
    if (shared_state_) {
      return ref_count_traits<T>::resolve(handle_());
    }
    return nullptr;
  }

  // ********** operator*() **********
  T& operator*() const noexcept {
    precondition(shared_state_, "accessing null pointer");

    return *ref_count_traits<T>::resolve(handle_());
  }

  // ********** operator->() **********
  T* operator->() const noexcept {
    precondition(shared_state_, "accessing null pointer");

    return ref_count_traits<T>::resolve(handle_());
  }

  // ********** use_count() **********
  long use_count() const noexcept {
    if (!shared_state_) {
      return 0;
    }
    return ref_count_traits<T>::use_count(handle_());
  }

  // ********** operator bool() **********
  explicit operator bool() const noexcept {
    return shared_state_ != nullptr;
  }

 private:
  void* handle_() const noexcept {
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
    return shared_state_;
  }

  void clear_() noexcept {
    shared_state_ = nullptr;
  }

  void add_ref_() noexcept {
    if (shared_state_) {
      ref_count_traits<T>::add_ref(handle_());
    }
  }

  void release_() noexcept {
    if (shared_state_) {
      ref_count_traits<T>::remove_ref(handle_());
    }
  }

  void* shared_state_ = nullptr;

  template <typename U, typename... Args>
  friend ref_count_ptr<U> make_ref_counted(Args&&... args);

  template <typename U>
  friend void swap(ref_count_ptr<U>& lhs, ref_count_ptr<U>& rhs) noexcept;

  template <typename U>
  friend class ref_count_ptr;
};

// ********** swap() **********
template <class T>
void swap(ref_count_ptr<T>& lhs, ref_count_ptr<T>& rhs) noexcept {
  std::swap(lhs.shared_state_, rhs.shared_state_);
}

template <typename T, typename... Args>
ref_count_ptr<T> make_ref_counted(Args&&... args) {
  ref_count_ptr<T> result;
  result.shared_state_ = ref_count_traits<T>::make_obj_and_shared_state(
      std::forward<Args>(args)...);
  return result;
}

template <class T, class U>
bool operator==(const ref_count_ptr<T>& lhs,
                const ref_count_ptr<U>& rhs) noexcept {
  return lhs.get() == rhs.get();
}

template <class T>
bool operator==(const ref_count_ptr<T>& lhs, std::nullptr_t) noexcept {
  return !lhs;
}

template <class T, class U>
std::strong_ordering operator<=>(const ref_count_ptr<T>& lhs,
                                 const ref_count_ptr<U>& rhs) noexcept {
  return std::compare_three_way{}(lhs.get(), rhs.get());
}

template <class T>
std::strong_ordering operator<=>(const ref_count_ptr<T>& lhs,
                                 std::nullptr_t) noexcept {
  return std::compare_three_way{}(
      lhs.get(),
      static_cast<typename ref_count_ptr<T>::element_type*>(nullptr));
}

}  // namespace abu::mem

#endif