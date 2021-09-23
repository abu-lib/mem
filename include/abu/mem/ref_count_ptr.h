// Copyright 2021 Francois Chabot

// Licensed under the Apache License, Version 2.0 (the "License");
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

template <typename T>
struct intrusively_ref_counted_traits;

class ref_counted {
  template <typename T>
  friend struct intrusively_ref_counted_traits;

  long ref_count_ = 0;

 protected:
  ~ref_counted() = default;
};

template <std::derived_from<ref_counted> T>
struct intrusively_ref_counted_traits<T> {
  static void add_ref(T* obj) {
    auto rc = static_cast<ref_counted*>(obj);
    rc->ref_count_ += 1;
  }

  static void remove_ref(T* obj) {
    auto rc = static_cast<ref_counted*>(obj);
    rc->ref_count_ -= 1;
    if (rc->ref_count_ == 0) {
      delete obj;
    }
  }

  static long use_count(T* obj) {
    auto rc = static_cast<ref_counted*>(obj);
    return rc->ref_count_;
  }
};

template <typename T>
concept IntrusivelyRefCouted = requires(T* obj) {
  intrusively_ref_counted_traits<T>::add_ref(obj);
  intrusively_ref_counted_traits<T>::remove_ref(obj);
  { intrusively_ref_counted_traits<T>::use_count(obj) } -> std::same_as<long>;
};

// Specialize this on whichever type you want to reference_count
namespace details_ {
  struct basic_shared_state {
    virtual ~basic_shared_state() = default;
    void* resolve() noexcept {
      return ptr;
    }

    long ref_count;
    void* ptr;
  };

  template <typename T>
  struct owned_shared_state : public basic_shared_state {
    template <typename... Args>
    owned_shared_state(Args&&... args) : obj(std::forward<Args>(args)...) {
      ptr = &obj;
    }
    T obj;
  };

  template <typename T>
  struct referenced_shared_state : public basic_shared_state {
    referenced_shared_state(T* init) {
      ptr = init;
    }

    ~referenced_shared_state() {
      delete reinterpret_cast<T*>(ptr);
    }
  };

  template <typename T>
  struct ref_count_traits {
    static void* create_shared_state(T* ptr) noexcept {
      assume(ptr);
      if constexpr (IntrusivelyRefCouted<T>) {
        intrusively_ref_counted_traits<T>::add_ref(ptr);
        return ptr;
      } else {
        auto bss = new referenced_shared_state<T>(ptr);
        bss->ref_count = 1;
        return bss;
      }
    }

    static void add_ref(void* shared_state) noexcept {
      assume(shared_state);
      if constexpr (IntrusivelyRefCouted<T>) {
        intrusively_ref_counted_traits<T>::add_ref(resolve(shared_state));
      } else {
        auto bss = reinterpret_cast<basic_shared_state*>(shared_state);
        bss->ref_count += 1;
      }
    }

    static void remove_ref(void* shared_state) noexcept {
      assume(shared_state);
      if constexpr (IntrusivelyRefCouted<T>) {
        intrusively_ref_counted_traits<T>::remove_ref(resolve(shared_state));
      } else {
        auto bss = reinterpret_cast<basic_shared_state*>(shared_state);
        assume(bss->ref_count > 0);
        bss->ref_count -= 1;
        if (bss->ref_count == 0) {
          delete bss;
        }
      }
    }

    static long use_count(void* shared_state) noexcept {
      assume(shared_state);
      if constexpr (IntrusivelyRefCouted<T>) {
        return intrusively_ref_counted_traits<T>::use_count(
            resolve(shared_state));
      } else {
        auto bss = reinterpret_cast<basic_shared_state*>(shared_state);
        return bss->ref_count;
      }
    }

    static T* resolve(void* shared_state) noexcept {
      assume(shared_state);
      if constexpr (IntrusivelyRefCouted<T>) {
        return static_cast<T*>(shared_state);
      } else {
        auto bss = reinterpret_cast<basic_shared_state*>(shared_state);
        return reinterpret_cast<T*>(bss->resolve());
      }
    }

    template <typename... Args>
    static void* make_obj_and_shared_state(Args&&... args) {
      if constexpr (IntrusivelyRefCouted<T>) {
        return create_shared_state(new T(std::forward<Args>(args)...));
      } else {
        auto bss = new owned_shared_state<T>(std::forward<Args>(args)...);
        bss->ref_count = 1;
        return bss;
      }
    }
  };

}  // namespace details_

// N.B. We want to make sure that linked lists of reference count are feasible.
template <typename T>
class ref_count_ptr {
 public:
  using element_type = T;

  // ********** Constructors **********
  ref_count_ptr() noexcept = default;
  ref_count_ptr(std::nullptr_t) noexcept {}

  explicit ref_count_ptr(T* ptr) noexcept {
    if (ptr) {
      shared_state_ = details_::ref_count_traits<T>::create_shared_state(ptr);
    }
  }

  template <std::derived_from<T> Y>
  explicit ref_count_ptr(Y* ptr) noexcept {
    if (ptr) {
      shared_state_ = details_::ref_count_traits<T>::create_shared_state(ptr);
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
      return details_::ref_count_traits<T>::resolve(shared_state_);
    }
    return nullptr;
  }

  // ********** operator*() **********
  T& operator*() const noexcept {
    precondition(shared_state_, "accessing null pointer");

    return *details_::ref_count_traits<T>::resolve(shared_state_);
  }

  // ********** operator->() **********
  T* operator->() const noexcept {
    precondition(shared_state_, "accessing null pointer");

    return details_::ref_count_traits<T>::resolve(shared_state_);
  }

  // ********** use_count() **********
  long use_count() const noexcept {
    if (!shared_state_) {
      return 0;
    }
    return details_::ref_count_traits<T>::use_count(shared_state_);
  }

  // ********** operator bool() **********
  explicit operator bool() const noexcept {
    return shared_state_ != nullptr;
  }

 private:
  void clear_() noexcept {
    shared_state_ = nullptr;
  }

  void add_ref_() noexcept {
    if (shared_state_) {
      details_::ref_count_traits<T>::add_ref(shared_state_);
    }
  }

  void release_() noexcept {
    if (shared_state_) {
      details_::ref_count_traits<T>::remove_ref(shared_state_);
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
  result.shared_state_ =
      details_::ref_count_traits<T>::make_obj_and_shared_state(
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