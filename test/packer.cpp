#include "packer.hpp"
#include "safe_int.hpp"

TEST(packer, SafeInt_Signed)
{
  constexpr intmax_t min = std::numeric_limits<int64_t>::min();
  constexpr intmax_t max = std::numeric_limits<int64_t>::max();
  using type = rdk::safe_signed<min, max>;
  static_assert(rdk::is_packable_v<type>, "SafeInt shall be packable");
  using traits = rdk::packable_traits<type>;
  std::uniform_int_distribution<type::value_type> dist(min, max);
  for(size_t i{}; i < 100000U; ++i)
  {
    type v{dist(rng)};
    auto &&packed = traits::pack(v);
    auto &&unpacked = traits::unpack(packed);
    ASSERT_EQ(v, unpacked) << traits::packed_size << ',' << static_cast<type::value_type>(v);
  }
}

TEST(packer, SafeInt_Unsigned)
{
  constexpr uintmax_t min = 0U;
  constexpr uintmax_t max = std::numeric_limits<uint64_t>::max();
  using type = rdk::safe_unsigned<min, max>;
  static_assert(rdk::is_packable_v<type>, "SafeInt shall be packable");
  using traits = rdk::packable_traits<type>;
  std::uniform_int_distribution<type::value_type> dist(min, max);
  for(size_t i{}; i < 100000U; ++i)
  {
    type v{dist(rng)};
    auto &&packed = traits::pack(v);
    auto &&unpacked = traits::unpack(packed);
    ASSERT_EQ(v, unpacked) << traits::packed_size << ',' << static_cast<type::value_type>(v);
  }
}
