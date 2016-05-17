/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>
          (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

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

#include "../port.h"

#include "enum.h"
#include "struct.h"
#include "optional.h"
#include "variant.h"
#include "timestamp.h"

#include "../Bricks/strings/strings.h"
#include "../Bricks/dflags/dflags.h"
#include "../3rdparty/gtest/gtest-main-with-dflags.h"

#include "Reflection/test.cc"
#include "Serialization/test.cc"
#include "Schema/test.cc"
#include "Evolution/test.cc"

namespace struct_definition_test {

// A few properly defined Current data types.
CURRENT_STRUCT(Foo) {
  CURRENT_FIELD(i, uint64_t, 0u);
  CURRENT_CONSTRUCTOR(Foo)(uint64_t i = 42u) : i(i) {}
  uint64_t twice_i() const { return i * 2; }
  bool operator<(const Foo& rhs) const { return i < rhs.i; }
};
CURRENT_STRUCT(Bar) {
  CURRENT_FIELD(j, uint64_t, 0u);
  CURRENT_CONSTRUCTOR(Bar)(uint64_t j = 43u) : j(j) {}
};
CURRENT_STRUCT(Baz) {
  CURRENT_FIELD(v1, std::vector<uint64_t>);
  CURRENT_FIELD(v2, std::vector<Foo>);
  CURRENT_FIELD(v3, std::vector<std::vector<Foo>>);
  CURRENT_FIELD(v4, (std::map<std::string, std::string>));
  CURRENT_FIELD(v5, (std::map<Foo, int>));
  CURRENT_FIELD_DESCRIPTION(v1, "We One.");
  CURRENT_FIELD_DESCRIPTION(v4, "We Four.");
};

CURRENT_STRUCT(DerivedFromFoo, Foo) {
  CURRENT_DEFAULT_CONSTRUCTOR(DerivedFromFoo) : SUPER(100u) {}
  CURRENT_CONSTRUCTOR(DerivedFromFoo)(size_t x) : SUPER(x * 1001u) {}
  CURRENT_FIELD(baz, Baz);
};

CURRENT_STRUCT(WithVector) {
  CURRENT_FIELD(v, std::vector<std::string>);
  size_t vector_size() const { return v.size(); }
};

static_assert(IS_VALID_CURRENT_STRUCT(Foo), "Struct `Foo` was not properly declared.");
static_assert(IS_VALID_CURRENT_STRUCT(Baz), "Struct `Baz` was not properly declared.");
static_assert(IS_VALID_CURRENT_STRUCT(DerivedFromFoo), "Struct `DerivedFromFoo` was not properly declared.");

}  // namespace struct_definition_test

// Confirm that Current data types defined in a namespace are accessible from outside it.
static_assert(IS_VALID_CURRENT_STRUCT(struct_definition_test::Foo), "Struct `Foo` was not properly declared.");
static_assert(IS_VALID_CURRENT_STRUCT(struct_definition_test::Baz), "Struct `Baz` was not properly declared.");
static_assert(IS_VALID_CURRENT_STRUCT(struct_definition_test::DerivedFromFoo),
              "Struct `DerivedFromFoo` was not properly declared.");

namespace some_other_namespace {

// Confirm that Current data types defined in one namespace are accessible from another one.
static_assert(IS_VALID_CURRENT_STRUCT(::struct_definition_test::Foo),
              "Struct `Foo` was not properly declared.");
static_assert(IS_VALID_CURRENT_STRUCT(::struct_definition_test::Baz),
              "Struct `Baz` was not properly declared.");
static_assert(IS_VALID_CURRENT_STRUCT(::struct_definition_test::DerivedFromFoo),
              "Struct `DerivedFromFoo` was not properly declared.");

}  // namespace some_other_namespace

namespace struct_definition_test {

// Properly declared structures.
CURRENT_STRUCT(Empty){};
CURRENT_STRUCT(EmptyDerived, Empty){};

static_assert(IS_VALID_CURRENT_STRUCT(Empty), "`Empty` must pass `IS_VALID_CURRENT_STRUCT` check.");
static_assert(IS_VALID_CURRENT_STRUCT(EmptyDerived),
              "`EmptyDerived` must pass `IS_VALID_CURRENT_STRUCT` check.");

// Improperly declared structures.
struct WrongStructNotCurrentStruct {
  int x;
};
struct WrongDerivedStructNotCurrentStruct : ::current::CurrentStruct {};
struct NotCurrentStructDerivedFromCurrentStruct : Empty {};

CURRENT_STRUCT(WrongUsesCOUNTERInternally) {
  CURRENT_FIELD(i1, uint64_t);
  static size_t GetCounter() { return __COUNTER__; }
  CURRENT_FIELD(i2, uint64_t);
};

// The lines below don't compile with various errors.
// static_assert(!IS_VALID_CURRENT_STRUCT(WrongStructNotCurrentStruct),
//               "`WrongStructNotCurrentStruct` must NOT pass `IS_VALID_CURRENT_STRUCT` check.");
// static_assert(!IS_VALID_CURRENT_STRUCT(WrongDerivedStructNotCurrentStruct),
//               "`WrongDerivedStructNotCurrentStruct` must NOT pass `IS_VALID_CURRENT_STRUCT` check.");
// static_assert(!IS_VALID_CURRENT_STRUCT(NotCurrentStructDerivedFromCurrentStruct),
//               "`NotCurrentStructDerivedFromCurrentStruct` must NOT pass `IS_VALID_CURRENT_STRUCT` check.");
// static_assert(!IS_VALID_CURRENT_STRUCT(WrongUsesCOUNTERInternally),
//               "`WrongUsesCOUNTERInternally` must not pass `IS_VALID_CURRENT_STRUCT` check.");

}  // namespace struct_definition_test

TEST(TypeSystemTest, FieldCounter) {
  using namespace struct_definition_test;
  {
    EXPECT_EQ(0, current::reflection::FieldCounter<Empty>::value);
    EXPECT_EQ(0, current::reflection::FieldCounter<EmptyDerived>::value);
    EXPECT_EQ(1, current::reflection::FieldCounter<Foo>::value);
    EXPECT_EQ(1, current::reflection::FieldCounter<Bar>::value);
    EXPECT_EQ(5, current::reflection::FieldCounter<Baz>::value);
    EXPECT_EQ(1, current::reflection::FieldCounter<DerivedFromFoo>::value);
  }
  {
    EXPECT_EQ(0, current::reflection::TotalFieldCounter<Empty>::value);
    EXPECT_EQ(0, current::reflection::TotalFieldCounter<EmptyDerived>::value);
    EXPECT_EQ(1, current::reflection::TotalFieldCounter<Foo>::value);
    EXPECT_EQ(1, current::reflection::TotalFieldCounter<Bar>::value);
    EXPECT_EQ(5, current::reflection::TotalFieldCounter<Baz>::value);
    EXPECT_EQ(2, current::reflection::TotalFieldCounter<DerivedFromFoo>::value);
  }
}

TEST(TypeSystemTest, FieldDescriptions) {
  using namespace struct_definition_test;
  EXPECT_STREQ("We One.", (current::reflection::FieldDescriptions::template Description<Baz, 0>()));
  EXPECT_STREQ(nullptr, (current::reflection::FieldDescriptions::template Description<Baz, 1>()));
  EXPECT_STREQ(nullptr, (current::reflection::FieldDescriptions::template Description<Baz, 2>()));
  EXPECT_STREQ("We Four.", (current::reflection::FieldDescriptions::template Description<Baz, 3>()));
  EXPECT_STREQ(nullptr, (current::reflection::FieldDescriptions::template Description<Baz, 4>()));
}

TEST(TypeSystemTest, ExistsAndValueSemantics) {
  {
    int x = 1;
    ASSERT_TRUE(Exists(x));
    EXPECT_EQ(1, Value(x));
    EXPECT_EQ(1, Value(Value(x)));
  }
  {
    const int x = 2;
    ASSERT_TRUE(Exists(x));
    EXPECT_EQ(2, Value(x));
    EXPECT_EQ(2, Value(Value(x)));
  }
  {
    int y = 3;
    int& x = y;
    ASSERT_TRUE(Exists(x));
    EXPECT_EQ(3, Value(x));
    EXPECT_EQ(3, Value(Value(x)));
    y = 4;
    ASSERT_TRUE(Exists(x));
    EXPECT_EQ(4, Value(x));
    EXPECT_EQ(4, Value(Value(x)));
  }
  {
    int y = 5;
    const int& x = y;
    ASSERT_TRUE(Exists(x));
    EXPECT_EQ(5, Value(x));
    EXPECT_EQ(5, Value(Value(x)));
    y = 6;
    ASSERT_TRUE(Exists(x));
    EXPECT_EQ(6, Value(x));
    EXPECT_EQ(6, Value(Value(x)));
  }
}

TEST(TypeSystemTest, ExistsForNonVariants) {
  using namespace struct_definition_test;

  Foo foo;
  EXPECT_TRUE(Exists<Foo>(foo));
  EXPECT_FALSE(Exists<Bar>(foo));
  EXPECT_FALSE(Exists<int>(foo));
  const Foo& foo_cref = foo;
  EXPECT_TRUE(Exists<Foo>(foo_cref));
  EXPECT_FALSE(Exists<Bar>(foo_cref));
  EXPECT_FALSE(Exists<int>(foo_cref));
  Foo& foo_ref = foo;
  EXPECT_TRUE(Exists<Foo>(foo_ref));
  EXPECT_FALSE(Exists<Bar>(foo_ref));
  Foo&& foo_rref = std::move(foo);
  EXPECT_TRUE(Exists<Foo>(foo_rref));
  EXPECT_FALSE(Exists<Bar>(foo_rref));
  EXPECT_FALSE(Exists<int>(foo_rref));

  EXPECT_TRUE(Exists<int>(42));
  EXPECT_FALSE(Exists<int>(foo));
  EXPECT_FALSE(Exists<int>(foo_cref));
  EXPECT_FALSE(Exists<int>(foo_ref));
  EXPECT_FALSE(Exists<int>(foo_rref));
}

TEST(TypeSystemTest, ValueOfMutableAndImmutableObjects) {
  using namespace struct_definition_test;

  Foo foo(42);
  EXPECT_TRUE(Exists<Foo>(foo));
  EXPECT_EQ(42ull, Value(foo).i);
  EXPECT_EQ(42ull, Value(Value(foo)).i);

  uint64_t& i_ref = Value<Foo&>(foo).i;
  i_ref += 10ull;
  EXPECT_EQ(52ull, Value(foo).i);
  EXPECT_EQ(52ull, Value(Value(foo)).i);

  const Foo& foo_cref1 = foo;
  const Foo& foo_cref2 = Value<const Foo&>(foo_cref1);
  const Foo& foo_cref3 = Value<const Foo&>(foo_cref2);

  ASSERT_TRUE(&foo_cref1 == &foo);
  ASSERT_TRUE(&foo_cref2 == &foo);
  ASSERT_TRUE(&foo_cref3 == &foo);

  const uint64_t& i_cref = foo_cref3.i;
  ASSERT_TRUE(&i_ref == &i_cref);

  EXPECT_EQ(52ull, foo_cref1.i);
  EXPECT_EQ(52ull, foo_cref2.i);
  EXPECT_EQ(52ull, foo_cref3.i);
  EXPECT_EQ(52ull, i_cref);

  ++i_ref;

  EXPECT_EQ(53ull, foo_cref1.i);
  EXPECT_EQ(53ull, foo_cref2.i);
  EXPECT_EQ(53ull, foo_cref3.i);
  EXPECT_EQ(53ull, i_cref);
}

TEST(TypeSystemTest, CopyDoesItsJob) {
  using namespace struct_definition_test;

  Foo a(1u);
  Foo b(a);
  EXPECT_EQ(1u, a.i);
  EXPECT_EQ(1u, b.i);

  a.i = 3u;
  EXPECT_EQ(3u, a.i);
  EXPECT_EQ(1u, b.i);

  Foo c;
  c = a;
  EXPECT_EQ(3u, a.i);
  EXPECT_EQ(3u, c.i);

  a.i = 5u;
  EXPECT_EQ(5u, a.i);
  EXPECT_EQ(3u, c.i);
}

TEST(TypeSystemTest, DerivedConstructorIsCalled) {
  using namespace struct_definition_test;
  DerivedFromFoo one;
  EXPECT_EQ(100u, one.i);
  DerivedFromFoo two(123u);
  EXPECT_EQ(123123u, two.i);
};

TEST(TypeSystemTest, ImmutableOptional) {
  {
    ImmutableOptional<int> foo(std::make_unique<int>(100));
    ASSERT_TRUE(Exists(foo));
    EXPECT_EQ(100, foo.ValueImpl());
    EXPECT_EQ(100, static_cast<int>(Value(foo)));
    EXPECT_EQ(100, static_cast<int>(Value(Value(foo))));
  }
  {
    ImmutableOptional<int> bar(nullptr);
    ASSERT_FALSE(Exists(bar));
    try {
      Value(bar);
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValue) {
    }
  }
  {
    ImmutableOptional<int> meh(nullptr);
    ASSERT_FALSE(Exists(meh));
    try {
      Value(meh);
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValueOfType<int>) {
    }
  }
  {
    struct_definition_test::Foo bare;
    ASSERT_TRUE(Exists(bare));
    EXPECT_EQ(42u, Value(bare).i);
    EXPECT_EQ(42u, Value(Value(bare)).i);
  }
  {
    struct_definition_test::Foo bare;
    ImmutableOptional<struct_definition_test::Foo> wrapped(FromBarePointer(), &bare);
    ASSERT_TRUE(Exists(wrapped));
    EXPECT_EQ(42u, wrapped.ValueImpl().i);
    EXPECT_EQ(42u, Value(wrapped).i);
    EXPECT_EQ(42u, Value(Value(wrapped)).i);
  }
  {
    struct_definition_test::Foo bare;
    ImmutableOptional<struct_definition_test::Foo> wrapped(FromBarePointer(), &bare);
    const ImmutableOptional<struct_definition_test::Foo>& double_wrapped(wrapped);
    ASSERT_TRUE(Exists(double_wrapped));
    EXPECT_EQ(42u, double_wrapped.ValueImpl().i);
    EXPECT_EQ(42u, Value(double_wrapped).i);
    EXPECT_EQ(42u, Value(Value(double_wrapped)).i);
  }
  {
    const auto lambda =
        [](int x) -> ImmutableOptional<int> { return ImmutableOptional<int>(std::make_unique<int>(x)); };
    ASSERT_TRUE(Exists(lambda(101)));
    EXPECT_EQ(102, lambda(102).ValueImpl());
    EXPECT_EQ(102, Value(lambda(102)));
  }
}

TEST(TypeSystemTest, Optional) {
  Optional<int> foo(std::make_unique<int>(200));
  ASSERT_TRUE(Exists(foo));
  EXPECT_EQ(200, foo.ValueImpl());
  EXPECT_EQ(200, Value(foo));
  foo = nullptr;
  ASSERT_FALSE(Exists(foo));
  try {
    Value(foo);
    ASSERT_TRUE(false);  // LCOV_EXCL_LINE
  } catch (NoValue) {
  }
}

namespace enum_class_test {
CURRENT_ENUM(Fruits, uint32_t){APPLE = 1u, ORANGE = 2u};
}  // namespace enum_class_test

TEST(TypeSystemTest, EnumRegistration) {
  using current::reflection::EnumName;
  EXPECT_EQ("Fruits", EnumName<enum_class_test::Fruits>());
}

TEST(TypeSystemTest, VariantStaticAsserts) {
  using namespace struct_definition_test;

  static_assert(is_same_or_compile_error<Variant<Foo>, Variant<Foo>>::value, "");
  static_assert(is_same_or_compile_error<Variant<Foo>, Variant<TypeList<Foo>>>::value, "");
  static_assert(is_same_or_compile_error<Variant<Foo>, Variant<SlowTypeList<Foo, Foo>>>::value, "");
  static_assert(is_same_or_compile_error<Variant<Foo>, Variant<TypeListImpl<Foo>>>::value, "");
  static_assert(Variant<Foo>::typelist_size == 1u, "");
  static_assert(is_same_or_compile_error<Variant<Foo>::typelist_t, TypeListImpl<Foo>>::value, "");

  static_assert(is_same_or_compile_error<Variant<Foo, Bar>, Variant<Foo, Bar>>::value, "");
  static_assert(is_same_or_compile_error<Variant<Foo, Bar>, Variant<TypeList<Foo, Bar>>>::value, "");
  static_assert(is_same_or_compile_error<Variant<Foo, Bar>, Variant<SlowTypeList<Foo, Bar, Foo>>>::value, "");
  static_assert(
      is_same_or_compile_error<Variant<Foo, Bar>, Variant<SlowTypeList<Foo, Bar, TypeList<Bar>>>>::value, "");
  static_assert(is_same_or_compile_error<Variant<Foo, Bar>, Variant<TypeListImpl<Foo, Bar>>>::value, "");
  static_assert(Variant<Foo, Bar>::typelist_size == 2u, "");
  static_assert(is_same_or_compile_error<Variant<Foo, Bar>::typelist_t, TypeListImpl<Foo, Bar>>::value, "");
}

TEST(TypeSystemTest, VariantCreateAndCopy) {
  using namespace struct_definition_test;

  // Move empty.
  {
    Variant<Foo, Bar> empty;
    Variant<Foo, Bar> moved(std::move(empty));
    EXPECT_FALSE(moved.ExistsImpl());
    EXPECT_FALSE(Exists(moved));
    EXPECT_FALSE(Exists<Foo>(moved));
    EXPECT_FALSE(Exists<Bar>(moved));
    EXPECT_FALSE((Exists<Variant<Foo, Bar>>(moved)));
  }
  {
    Variant<Foo, Bar> empty;
    Variant<Foo, Bar> moved;
    moved = std::move(empty);
    EXPECT_FALSE(moved.ExistsImpl());
    EXPECT_FALSE(Exists(moved));
    EXPECT_FALSE(Exists<Foo>(moved));
    EXPECT_FALSE(Exists<Bar>(moved));
    EXPECT_FALSE((Exists<Variant<Foo, Bar>>(moved)));
  }

  // Copy empty.
  {
    Variant<Foo, Bar> empty;
    Variant<Foo, Bar> copied(empty);
    EXPECT_FALSE(copied.ExistsImpl());
    EXPECT_FALSE(Exists(copied));
    EXPECT_FALSE(Exists<Foo>(copied));
    EXPECT_FALSE(Exists<Bar>(copied));
    EXPECT_FALSE((Exists<Variant<Foo, Bar>>(copied)));
  }
  {
    Variant<Foo, Bar> empty;
    Variant<Foo, Bar> copied;
    copied = empty;
    EXPECT_FALSE(copied.ExistsImpl());
    EXPECT_FALSE(Exists(copied));
    EXPECT_FALSE(Exists<Foo>(copied));
    EXPECT_FALSE(Exists<Bar>(copied));
    EXPECT_FALSE((Exists<Variant<Foo, Bar>>(copied)));
  }

  // Move non-empty.
  {
    Variant<Foo, Bar> foo(Foo(100u));
    EXPECT_TRUE(Exists(foo));
    EXPECT_TRUE(Exists<Foo>(foo));
    EXPECT_FALSE(Exists<Bar>(foo));
    Variant<Foo, Bar> moved(std::move(foo));
    EXPECT_TRUE(Exists(moved));
    EXPECT_TRUE(Exists<Foo>(moved));
    EXPECT_FALSE(Exists<Bar>(moved));
    EXPECT_EQ(100u, Value<Foo>(moved).i);
    EXPECT_TRUE((Exists<Variant<Foo, Bar>>(moved)));
    EXPECT_EQ(100u, (Value<Foo>(Value<Variant<Foo, Bar>>(moved)).i));
  }
  {
    Variant<Foo, Bar> foo(Foo(101u));
    Variant<Foo, Bar> moved;
    moved = std::move(foo);
    EXPECT_TRUE(Exists(moved));
    EXPECT_FALSE(Exists(foo));
    EXPECT_TRUE(Exists<Foo>(moved));
    EXPECT_FALSE(Exists<Bar>(moved));
    EXPECT_EQ(101u, Value<Foo>(moved).i);
    EXPECT_TRUE((Exists<Variant<Foo, Bar>>(moved)));
    EXPECT_EQ(101u, (Value<Foo>(Value<Variant<Foo, Bar>>(moved)).i));
  }

  // Copy non-empty.
  {
    Variant<Foo, Bar> foo(Foo(100u));
    Variant<Foo, Bar> copied(foo);
    Value<Foo>(foo).i = 101u;
    EXPECT_TRUE(Exists(copied));
    EXPECT_TRUE(Exists<Foo>(copied));
    EXPECT_FALSE(Exists<Bar>(copied));
    EXPECT_EQ(100u, Value<Foo>(copied).i);
    EXPECT_EQ(100u, Value<Foo>(copied).i);
    EXPECT_EQ(101u, Value<Foo>(foo).i);
    EXPECT_TRUE((Exists<Variant<Foo, Bar>>(copied)));
    EXPECT_EQ(100u, (Value<Foo>(Value<Variant<Foo, Bar>>(copied)).i));
    EXPECT_EQ(101u, (Value<Foo>(Value<Variant<Foo, Bar>>(foo)).i));
  }
  {
    Variant<Foo, Bar> bar(Bar(100u));
    Variant<Foo, Bar> copied;
    copied = bar;
    EXPECT_TRUE(Exists(copied));
    EXPECT_FALSE(Exists<Foo>(copied));
    EXPECT_TRUE(Exists<Bar>(copied));
    Value<Bar>(bar).j = 101u;
    EXPECT_EQ(100u, Value<Bar>(copied).j);
    EXPECT_EQ(101u, Value<Bar>(bar).j);
    EXPECT_TRUE((Exists<Variant<Foo, Bar>>(copied)));
    EXPECT_EQ(100u, (Value<Bar>(Value<Variant<Foo, Bar>>(copied)).j));
    EXPECT_EQ(101u, (Value<Bar>(Value<Variant<Foo, Bar>>(bar)).j));
  }
}

TEST(TypeSystemTest, VariantSmokeTestOneType) {
  using namespace struct_definition_test;

  {
    Variant<Foo> p(std::make_unique<Foo>());
    const Variant<Foo>& cp(p);

    {
      ASSERT_TRUE(p.VariantExistsImpl<Foo>());
      const auto& foo = p.VariantValueImpl<Foo>();
      EXPECT_EQ(42u, foo.i);
    }
    {
      ASSERT_TRUE(cp.VariantExistsImpl<Foo>());
      const auto& foo = cp.VariantValueImpl<Foo>();
      EXPECT_EQ(42u, foo.i);
    }
    {
      ASSERT_TRUE(p.VariantExistsImpl<Foo>());
      const auto& foo = Value<Foo>(p);
      EXPECT_EQ(42u, foo.i);
    }
    {
      ASSERT_TRUE(cp.VariantExistsImpl<Foo>());
      const auto& foo = Value<Foo>(cp);
      EXPECT_EQ(42u, foo.i);
    }

    ++p.VariantValueImpl<Foo>().i;

    EXPECT_EQ(43u, p.VariantValueImpl<Foo>().i);
    EXPECT_EQ(43u, cp.VariantValueImpl<Foo>().i);

    p = Foo(100u);
    EXPECT_EQ(100u, p.VariantValueImpl<Foo>().i);
    EXPECT_EQ(100u, cp.VariantValueImpl<Foo>().i);

    p = static_cast<const Foo&>(Foo(101u));
    EXPECT_EQ(101u, p.VariantValueImpl<Foo>().i);
    EXPECT_EQ(101u, cp.VariantValueImpl<Foo>().i);

    p = Foo(102u);
    EXPECT_EQ(102u, p.VariantValueImpl<Foo>().i);
    EXPECT_EQ(102u, cp.VariantValueImpl<Foo>().i);

    p = std::make_unique<Foo>(103u);
    EXPECT_EQ(103u, p.VariantValueImpl<Foo>().i);
    EXPECT_EQ(103u, cp.VariantValueImpl<Foo>().i);
  }

  {
    struct Visitor {
      std::string s;
      void operator()(const Foo& foo) { s += "Foo " + current::ToString(foo.i) + '\n'; }
    };
    Visitor v;
    {
      Variant<Foo> p(Foo(501u));
      p.Call(v);
      EXPECT_EQ("Foo 501\n", v.s);
    }
    {
      const Variant<Foo> p(Foo(502u));
      p.Call(v);
      EXPECT_EQ("Foo 501\nFoo 502\n", v.s);
    }
  }

  {
    std::string s;
    const auto lambda = [&s](const Foo& foo) { s += "lambda: Foo " + current::ToString(foo.i) + '\n'; };
    {
      Variant<Foo> p(Foo(601u));
      p.Call(lambda);
      EXPECT_EQ("lambda: Foo 601\n", s);
    }
    {
      const Variant<Foo> p(Foo(602u));
      p.Call(lambda);
      EXPECT_EQ("lambda: Foo 601\nlambda: Foo 602\n", s);
    }
  }

  {
    const Variant<Foo> p((Foo()));
    try {
      p.VariantValueImpl<Bar>();
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValue) {
    }
    try {
      p.VariantValueImpl<Bar>();
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValueOfType<Bar>) {
    }
    try {
      Value<Bar>(p);
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValue) {
    }
    try {
      Value<Bar>(p);
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValueOfType<Bar>) {
    }
  }
}

TEST(TypeSystemTest, VariantSmokeTestMultipleTypes) {
  using namespace struct_definition_test;

  struct Visitor {
    std::string s;
    void operator()(const Bar&) { s = "Bar"; }
    void operator()(const Foo& foo) { s = "Foo " + current::ToString(foo.i); }
    void operator()(const DerivedFromFoo& object) {
      s = "DerivedFromFoo [" + current::ToString(object.baz.v1.size()) + "]";
    }
  };
  Visitor v;

  {
    Variant<Bar, Foo, DerivedFromFoo> p((Bar()));
    const Variant<Bar, Foo, DerivedFromFoo>& cp = p;

    p.Call(v);
    EXPECT_EQ("Bar", v.s);
    cp.Call(v);
    EXPECT_EQ("Bar", v.s);

    p = Foo(1u);

    p.Call(v);
    EXPECT_EQ("Foo 1", v.s);
    cp.Call(v);
    EXPECT_EQ("Foo 1", v.s);

    try {
      p.VariantValueImpl<Bar>();
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValue) {
    }
    try {
      p.VariantValueImpl<Bar>();
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValueOfType<Bar>) {
    }

    p = std::make_unique<DerivedFromFoo>();
    p.Call(v);
    EXPECT_EQ("DerivedFromFoo [0]", v.s);
    cp.Call(v);
    EXPECT_EQ("DerivedFromFoo [0]", v.s);

    p.VariantValueImpl<DerivedFromFoo>().baz.v1.resize(3);
    p.Call(v);
    EXPECT_EQ("DerivedFromFoo [3]", v.s);
    cp.Call(v);
    EXPECT_EQ("DerivedFromFoo [3]", v.s);

    try {
      p.VariantValueImpl<Bar>();
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValue) {
    }
    try {
      p.VariantValueImpl<Bar>();
      ASSERT_TRUE(false);  // LCOV_EXCL_LINE
    } catch (NoValueOfType<Bar>) {
    }
  }
}

TEST(TypeSystemTest, NestedVariants) {
  using namespace struct_definition_test;

  using V_FOO_DERIVED = Variant<Foo, DerivedFromFoo>;
  using V_BAR_BAZ = Variant<Bar, Baz>;
  using V_NESTED = Variant<V_FOO_DERIVED, V_BAR_BAZ>;

  V_FOO_DERIVED foo(Foo(1u));
  V_NESTED nested_foo(foo);
  EXPECT_EQ(1u, Value<Foo>(Value<V_FOO_DERIVED>(nested_foo)).i);

  V_BAR_BAZ bar(Bar(2u));
  V_NESTED nested_bar(bar);
  EXPECT_EQ(2u, Value<Bar>(Value<V_BAR_BAZ>(nested_bar)).j);
}

namespace struct_definition_test {
CURRENT_STRUCT(WithTimestampUS) {
  CURRENT_FIELD(t, std::chrono::microseconds);
  CURRENT_USE_FIELD_AS_TIMESTAMP(t);
};
CURRENT_STRUCT(WithTimestampUInt64) {
  CURRENT_FIELD(another_t, int64_t);
  CURRENT_USE_FIELD_AS_TIMESTAMP(another_t);
};
}  // namespace struct_definition_test

TEST(TypeSystemTest, TimestampSimple) {
  using namespace struct_definition_test;
  {
    WithTimestampUS a;
    a.t = std::chrono::microseconds(42ull);
    EXPECT_EQ(42ll, MicroTimestampOf(a).count());
    EXPECT_EQ(42ll, MicroTimestampOf(const_cast<const WithTimestampUS&>(a)).count());
  }
  {
    WithTimestampUInt64 x;
    x.another_t = 43ull;
    EXPECT_EQ(43ll, MicroTimestampOf(x).count());
    EXPECT_EQ(43ll, MicroTimestampOf(const_cast<const WithTimestampUInt64&>(x)).count());
  }
}

namespace struct_definition_test {
CURRENT_STRUCT(WithTimestampVariant) {
  CURRENT_FIELD(magic, (Variant<WithTimestampUS, WithTimestampUInt64>));
  CURRENT_CONSTRUCTOR(WithTimestampVariant)(const WithTimestampUS& magic) : magic(magic) {}
  CURRENT_CONSTRUCTOR(WithTimestampVariant)(const WithTimestampUInt64& magic) : magic(magic) {}
  CURRENT_USE_FIELD_AS_TIMESTAMP(magic);
};
}  // namespace struct_definition_test

TEST(TypeSystemTest, TimestampVariant) {
  using namespace struct_definition_test;

  WithTimestampUS a;
  a.t = std::chrono::microseconds(101);
  WithTimestampUInt64 b;
  b.another_t = 102ull;

  WithTimestampVariant z1(a);
  EXPECT_EQ(101ll, MicroTimestampOf(z1).count());
  z1.magic.VariantValueImpl<WithTimestampUS>().t = std::chrono::microseconds(201);
  EXPECT_EQ(201ll, MicroTimestampOf(z1).count());

  WithTimestampVariant z2(b);
  EXPECT_EQ(102ll, MicroTimestampOf(z2).count());
  z2.magic.VariantValueImpl<WithTimestampUInt64>().another_t = 202ull;
  EXPECT_EQ(202ll, MicroTimestampOf(z2).count());
}

TEST(TypeSystemTest, ConstructorsAndMemberFunctions) {
  using namespace struct_definition_test;
  {
    Foo foo(42u);
    EXPECT_EQ(84u, foo.twice_i());
  }
  {
    WithVector v;
    EXPECT_EQ(0u, v.vector_size());
    v.v.push_back("foo");
    v.v.push_back("bar");
    EXPECT_EQ(2u, v.vector_size());
  }
}

namespace struct_definition_test {

CURRENT_STRUCT(WithVariant) {
  CURRENT_FIELD(p, (Variant<Foo, Bar>));
  CURRENT_CONSTRUCTOR(WithVariant)(Foo foo) : p(foo) {}
};

}  // namespace struct_definition_test

TEST(TypeSystemTest, ComplexCloneCases) {
  using namespace struct_definition_test;

  {
    Variant<Foo, Bar> x(Foo(1));
    Variant<Foo, Bar> y = Clone(x);
    EXPECT_EQ(1u, Value<Foo>(y).i);
  }

  {
    WithVariant x(Foo(2));
    WithVariant y = Clone(x);
    EXPECT_EQ(2u, Value<Foo>(y.p).i);
  }
}
