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

#include <memory>

#include "abu/mem.h"
#include "gtest/gtest.h"

using namespace abu;

TEST(ref_counted, pointer_of_arithmetic_type) {
  mem::ref_count_ptr<int> x;
  mem::ref_count_ptr<int> y{new int(5)};
  auto z = abu::mem::make_ref_counted<int>(4);
  auto w = z;

  EXPECT_EQ(z.use_count(), 2);

  EXPECT_EQ(*y, 5);
  EXPECT_EQ(*z, 4);
  swap(y, z);

  EXPECT_EQ(*z, 5);
  EXPECT_EQ(*y, 4);
}

TEST(ref_counted, object_gets_deleted) {
  struct ObjType : public mem::ref_counted {
    ObjType(int& tgt) : tgt_(tgt) {
      tgt_ = 1;
    }
    ~ObjType() {
      tgt_ = 2;
    }
    int& tgt_;
  };

  int v = 0;
  {
    mem::ref_count_ptr<ObjType> y;
    {
      mem::ref_count_ptr<ObjType> x = mem::make_ref_counted<ObjType>(v);
      EXPECT_EQ(1, v);

      y = x;
    }
    EXPECT_EQ(1, v);
  }
  EXPECT_EQ(2, v);
}

TEST(ref_counted, default_pointer) {
  struct ObjType : public mem::ref_counted {
    void foo() {}
  };
  ObjType* raw_ptr = nullptr;

  mem::ref_count_ptr<ObjType> x;
  mem::ref_count_ptr<ObjType> y = nullptr;
  mem::ref_count_ptr<ObjType> z{raw_ptr};
  mem::ref_count_ptr<ObjType> w{x};

  EXPECT_EQ(x.use_count(), 0);
  EXPECT_EQ(x, y);
  EXPECT_EQ(x, z);
  EXPECT_EQ(x, nullptr);
  EXPECT_EQ(nullptr, x);

#if ABU_MEM_PRECONDITIONS
  EXPECT_DEATH(*x, "null");
  EXPECT_DEATH(x->foo(), "null");
#endif
}

TEST(ref_counted, init_base_from_derived_raw_ptr) {
  struct Base : public mem::ref_counted {
    virtual ~Base() = default;
  };
  struct Derived : public Base {};

  Derived* raw_derived = nullptr;
  mem::ref_count_ptr<Base> dx{raw_derived};

  Derived* raw_derived_2 = new Derived();
  mem::ref_count_ptr<Base> dx2{raw_derived_2};

  EXPECT_FALSE(dx);
  EXPECT_TRUE(dx2);
}

TEST(ref_counted, move_and_copy_pointers) {
  struct ObjType : public mem::ref_counted {
    virtual ~ObjType() = default;
  };
  struct Derived : public ObjType {};

  mem::ref_count_ptr<Derived> dx = mem::make_ref_counted<Derived>();
  mem::ref_count_ptr<ObjType> dxb;
  dxb = dx;
  EXPECT_EQ(dx, dxb);

  mem::ref_count_ptr<ObjType> x = mem::make_ref_counted<ObjType>();

  auto ptr = x.get();

  mem::ref_count_ptr<ObjType> y = std::move(x);
  EXPECT_FALSE(x);
  EXPECT_EQ(y.get(), ptr);

  x = std::move(y);
  EXPECT_FALSE(y);
  EXPECT_EQ(x.get(), ptr);

  y = x;
  EXPECT_TRUE(y);
  EXPECT_TRUE(x);
  EXPECT_EQ(x, y);

  x.reset();
  swap(x, y);
}

TEST(ref_counted, compare) {
  struct Base : public mem::ref_counted {
    virtual ~Base() = default;
  };

  struct Derived : public Base {};

  mem::ref_count_ptr<Base> x = mem::make_ref_counted<Base>();
  mem::ref_count_ptr<Base> y = mem::make_ref_counted<Base>();
  mem::ref_count_ptr<Derived> z = mem::make_ref_counted<Derived>();

  EXPECT_EQ(x > y, x.get() > y.get());
  EXPECT_EQ(x >= y, x.get() >= y.get());
  EXPECT_EQ(x < y, x.get() < y.get());
  EXPECT_EQ(x <= y, x.get() <= y.get());
  EXPECT_EQ(x != y, x.get() != y.get());
  EXPECT_EQ(x == y, x.get() == y.get());
  EXPECT_EQ(x <=> y, x.get() <=> y.get());

  EXPECT_EQ(x > z, x.get() > z.get());
  EXPECT_EQ(x >= z, x.get() >= z.get());
  EXPECT_EQ(x < z, x.get() < z.get());
  EXPECT_EQ(x <= z, x.get() <= z.get());
  EXPECT_EQ(x != z, x.get() != z.get());
  EXPECT_EQ(x == z, x.get() == z.get());
  EXPECT_EQ(x <=> z, x.get() <=> z.get());

  Base* raw_ptr = nullptr;
  EXPECT_EQ(x <=> nullptr, x.get() <=> raw_ptr);
}

TEST(ref_counted, polymorphic_destruction) {
  struct Base : public mem::ref_counted {
    virtual ~Base() = default;
  };

  struct Derived final : public Base {};

  mem::ref_count_ptr<Base> x = mem::make_ref_counted<Base>();
  mem::ref_count_ptr<Base> y = mem::make_ref_counted<Derived>();
  mem::ref_count_ptr<Derived> z = mem::make_ref_counted<Derived>();
}

TEST(ref_counted, swappable) {
  struct ObjType : public mem::ref_counted {};

  mem::ref_count_ptr<ObjType> x = mem::make_ref_counted<ObjType>();
  mem::ref_count_ptr<ObjType> y = mem::make_ref_counted<ObjType>();
  auto x_b = x;

  auto ptr_x = x.get();
  auto ptr_y = y.get();

  std::swap(x, y);

  EXPECT_EQ(ptr_x, y.get());
  EXPECT_EQ(ptr_y, x.get());

  EXPECT_EQ(x.use_count(), 1);
  EXPECT_EQ(y.use_count(), 2);
}

TEST(ref_counted, compatible_pointers) {
  struct Base : public mem::ref_counted {
    virtual ~Base() = default;
    virtual void foo() = 0;
  };

  struct Derived : public Base {
    void foo() override {}
  };

  auto derived_ptr = mem::make_ref_counted<Derived>();
  auto derived_ptr_b = derived_ptr;

  mem::ref_count_ptr<Base> base_ptr_a{derived_ptr.get()};
  mem::ref_count_ptr<Base> base_ptr_b{derived_ptr};
  mem::ref_count_ptr<Base> base_ptr_c{base_ptr_a};
  mem::ref_count_ptr<Base> base_ptr_moved{std::move(derived_ptr_b)};

  (*derived_ptr).foo();
  (*base_ptr_a).foo();
  derived_ptr->foo();
  base_ptr_a->foo();

  EXPECT_FALSE(derived_ptr_b);

  EXPECT_EQ(derived_ptr, base_ptr_a);
  EXPECT_EQ(base_ptr_b, derived_ptr);
  EXPECT_EQ(base_ptr_a, base_ptr_b);
  EXPECT_EQ(base_ptr_a, base_ptr_moved);

  EXPECT_EQ(derived_ptr.use_count(), 5);
  EXPECT_EQ(base_ptr_a.use_count(), 5);
  EXPECT_EQ(base_ptr_b.use_count(), 5);

  mem::ref_count_ptr<Base> base_ptr_d = std::move(base_ptr_a);
  EXPECT_EQ(base_ptr_a, nullptr);
  EXPECT_FALSE(base_ptr_a);
  EXPECT_EQ(base_ptr_c.use_count(), 5);

  base_ptr_a = std::move(derived_ptr);
  EXPECT_EQ(base_ptr_a.use_count(), 5);
  EXPECT_FALSE(derived_ptr);

  base_ptr_a.reset();
  EXPECT_EQ(base_ptr_c.use_count(), 4);
}

TEST(ref_counted, can_setup_linked_list) {
  struct node {
    mem::ref_count_ptr<node> next;
  };

  auto ptr = mem::make_ref_counted<node>();
  ptr->next = mem::make_ref_counted<node>();
}

TEST(ref_counted, implicitly_ref_counted) {
  struct Base {
    virtual ~Base() = default;
    virtual void foo() = 0;
  };

  struct Derived : public Base {
    void foo() override {}
  };

  struct Unrelated {
    void foo() {}
  };

  auto derived_ptr = mem::make_ref_counted<Derived>();
  mem::ref_count_ptr<Base> base_ptr = derived_ptr;
  static_assert(!std::is_convertible_v<mem::ref_count_ptr<Derived>,
                                       mem::ref_count_ptr<Unrelated>>);
}