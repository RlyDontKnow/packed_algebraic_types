#include "safe_int.hpp"

#include <type_traits>

namespace
{
  template<typename T, bool = std::is_signed_v<T>>
  struct TestDeductionImpl;

  template<typename T>
  struct TestDeductionImpl<T, true>
  {
    static void call()
    {
      constexpr T min = std::numeric_limits<T>::min();
      constexpr T max = std::numeric_limits<T>::max();
      EXPECT_TRUE((std::is_same_v<rdk::safe<T, min, max>, rdk::safe_signed<min, max>>));
    }
  };

  template<typename T>
  struct TestDeductionImpl<T, false>
  {
    static void call()
    {
      constexpr T min = std::numeric_limits<T>::min();
      constexpr T max = std::numeric_limits<T>::max();
      EXPECT_TRUE((std::is_same_v<rdk::safe<T, min, max>, rdk::safe_unsigned<min, max>>));
    }
  };

  template<typename T>
  void TestDeduction()
  {
    TestDeductionImpl<T>::call();
  }
}

TEST(SafeInt, DeduceTypes)
{
  TestDeduction<int8_t>();
  TestDeduction<int16_t>();
  TestDeduction<int32_t>();
  TestDeduction<int64_t>();
  TestDeduction<uint8_t>();
  TestDeduction<uint16_t>();
  TestDeduction<uint32_t>();
  TestDeduction<uint64_t>();
}

TEST(SafeInt, Add)
{
  rdk::safe_signed<0, 5> v1{3};
  rdk::safe_signed<-0x8000, 0x7FFF> v2{-127};
  rdk::safe_unsigned<0, 0x7F> v3{12};
  rdk::safe_unsigned<0x1000, 0x7FFF> v4{0x2000};

  auto r1 = v1 + v2;
  EXPECT_EQ(-127 + 3, static_cast<decltype(r1)::value_type>(r1));
  auto r2 = v3 + v4;
  EXPECT_EQ(0x2000 + 12, static_cast<decltype(r2)::value_type>(r2));
  auto r3 = v1 + v4;
  EXPECT_EQ(0x2000 + 3, static_cast<decltype(r3)::value_type>(r3));
  auto r4 = v3 + v2;
  EXPECT_EQ(-127 + 12, static_cast<decltype(r4)::value_type>(r4));
}

TEST(SafeInt, Sub)
{
  rdk::safe_signed<0, 5> v1{3};
  rdk::safe_signed<-0x8000, 0x7FFF> v2{-127};
  rdk::safe_unsigned<0, 0x7F> v3{12};
  rdk::safe_unsigned<0x1000, 0x7FFF> v4{0x2000};

  auto r1 = v1 - v2;
  EXPECT_EQ(3 - -127, static_cast<decltype(r1)::value_type>(r1));
  auto r2 = v3 - v4;
  EXPECT_EQ(12 - 0x2000, static_cast<decltype(r2)::value_type>(r2));
  auto r3 = v1 - v4;
  EXPECT_EQ(3 - 0x2000, static_cast<decltype(r3)::value_type>(r3));
  auto r4 = v3 - v2;
  EXPECT_EQ(12 - -127, static_cast<decltype(r4)::value_type>(r4));
}
