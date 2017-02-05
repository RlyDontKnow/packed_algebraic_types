#pragma once
#ifndef RDK_962F9BEDF2C749EDAD5BFB085F2951F7
#define RDK_962F9BEDF2C749EDAD5BFB085F2951F7

#include "packer.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace rdk
{

struct unchecked_construct_t
{
};
constexpr unchecked_construct_t unchecked_construct{};

template<typename T, T min, T max>
class safe
{
public:
  using value_type = T;

  /// no default construction
  /// use optional<safe<...>> to get a default constructible
  /// type at the expense of one bit storage
  safe() = delete;

  constexpr explicit safe(T v)
    : v(v)
  {
    if((v < min) || (v > max))
    {
      throw std::domain_error("SafeInt: value exceeds specified range.");
    }
  }

  /// unchecked construction; asserts the value is in range instead of checking it and throwing on error
  /// should only be used when it's known statically that the value is in range
  constexpr explicit safe(T v, unchecked_construct_t) noexcept
    : v(v)
  {
    assert((v >= min) && (v <= max));
  }

  template<typename U, U min2, U max2>
  constexpr safe(safe<U, min2, max2> const &other) noexcept((min2 >= min) && (max2 <= max))
  {
    static_assert((min <= max2) && (max >= min2), "SafeInt: value cannot be constructed from the specified type");

    constexpr bool bad_cast = false
      || ((min2 < min) && (other.v < min))
      || ((max2 > max) && (other.v > max))
      ;

    if(bad_cast)
    {
      throw std::domain_error("SafeInt: value exceeds specified range.");
    }

    v = static_cast<T>(other.v);
  }

  constexpr explicit operator T() const noexcept
  {
    return v;
  }

  constexpr safe &operator++()
  {
    if(max == v)
    {
      throw std::domain_error("SafeInt: value exceeds specified range.");
    }

    (void)++v;
    return *this;
  }

  constexpr safe operator++(int)
  {
    auto res = *this;
    (void)++*this;
    return res;
  }

  constexpr safe &operator--()
  {
    if(min == v)
    {
      throw std::domain_error("SafeInt: value exceeds specified range.");
    }

    (void)--v;
    return *this;
  }

  constexpr safe operator--(int)
  {
    auto res = *this;
    (void)--*this;
    return res;
  }

  constexpr safe operator+() const noexcept
  {
    return{*this};
  }

  constexpr auto operator-() const noexcept
  {
    return (safe<T, T{}, T{}>{T{}, unchecked_construct} - *this);
  }

  // @@@TODO: do we want to define operator~ for unsigned types? (could be awkward with deduced underlying types)

private:
  static_assert(std::is_integral_v<T>, "SafeInt: underlying storage must be an integral type");
  static_assert(min <= max, "SafeInt: value range mustn't be empty");

  template<class U, U, U>
  friend class safe;

  T v;
};

// is_safe trait
namespace detail
{
  template<typename T>
  struct is_safe : std::false_type
  {
  };

  template<typename T, T min, T max>
  struct is_safe<safe<T, min, max>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool is_safe_v = is_safe<T>::value;
} // namespace detail

// type deduction helpers
namespace detail
{
  template<intmax_t min, intmax_t max>
  struct signed_type_from_range
  {
    using type = std::conditional_t
    <
      ((min >= std::numeric_limits<int8_t>::min()) && (max <= std::numeric_limits<int8_t>::max()))
    , int8_t
    , std::conditional_t
      <
        ((min >= std::numeric_limits<int16_t>::min()) && (max <= std::numeric_limits<int16_t>::max()))
      , int16_t
      , std::conditional_t
        <
          ((min >= std::numeric_limits<int32_t>::min()) && (max <= std::numeric_limits<int32_t>::max()))
        , int32_t
        , std::conditional_t
          <
            ((min >= std::numeric_limits<int64_t>::min()) && (max <= std::numeric_limits<int64_t>::max()))
          , int64_t
          , void
          >
        >
      >
    >;
  };

  template<uintmax_t min, uintmax_t max>
  struct unsigned_type_from_range
  {
    using type = std::conditional_t
    <
      ((min >= std::numeric_limits<uint8_t>::min()) && (max <= std::numeric_limits<uint8_t>::max()))
    , uint8_t
    , std::conditional_t
      <
        ((min >= std::numeric_limits<uint16_t>::min()) && (max <= std::numeric_limits<uint16_t>::max()))
      , uint16_t
      , std::conditional_t
        <
          ((min >= std::numeric_limits<uint32_t>::min()) && (max <= std::numeric_limits<uint32_t>::max()))
        , uint32_t
        , std::conditional_t
          <
            ((min >= std::numeric_limits<uint64_t>::min()) && (max <= std::numeric_limits<uint64_t>::max()))
          , uint64_t
          , void
          >
        >
      >
    >;
  };
} // namespace detail

template<intmax_t min, intmax_t max>
using safe_signed = safe<typename detail::signed_type_from_range<min, max>::type, min, max>;

template<uintmax_t min, uintmax_t max>
using safe_unsigned = safe<typename detail::unsigned_type_from_range<min, max>::type, min, max>;

// comparison operators
namespace detail
{
  template<typename T, typename U, bool = std::is_signed_v<T>, bool = std::is_signed_v<U>>
  struct comp;

  template<typename T, typename U>
  struct comp<T, U, false, false>
  {
    static constexpr bool call(T lhs, U rhs)
    {
      return (lhs < rhs);
    }
  };

  template<typename T, typename U>
  struct comp<T, U, true, true>
  {
    static constexpr bool call(T lhs, U rhs)
    {
      return (lhs < rhs);
    }
  };

  template<typename T, typename U>
  struct comp<T, U, true, false>
  {
    static constexpr bool call(T lhs, U rhs)
    {
      return (lhs < T{}) ? true : (static_cast<std::make_unsigned_t<T>>(lhs) < rhs);
    }
  };

  template<typename T, typename U>
  struct comp<T, U, false, true>
  {
    static constexpr bool call(T lhs, U rhs)
    {
      return (rhs < U{}) ? false : (lhs < static_cast<std::make_unsigned_t<U>>(rhs));
    }
  };
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr bool operator<(T lhs, U rhs)
{
  return detail::comp<typename T::value_type, typename U::value_type>::call(
    static_cast<typename T::value_type>(lhs), static_cast<typename U::value_type>(rhs));
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr bool operator>(T lhs, U rhs)
{
  return (rhs < lhs);
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr bool operator>=(T lhs, U rhs)
{
  return !(lhs < rhs);
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr bool operator<=(T lhs, U rhs)
{
  return !(lhs >= rhs);
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr bool operator==(T lhs, U rhs)
{
  return (!(lhs < rhs) && !(rhs < lhs));
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr bool operator!=(T lhs, U rhs)
{
  return !(lhs == rhs);
}

// addition helpers
namespace detail
{
  template
  <
    typename T
  , typename U
  , bool = (std::is_signed_v<typename T::value_type> || std::is_signed_v<typename U::value_type>)
  >
  struct add;

  template<typename T, typename U>
  struct add<T, U, true>
  {
    using limits_lhs = std::numeric_limits<T>;
    using limits_rhs = std::numeric_limits<U>;

    static constexpr auto min = std::numeric_limits<intmax_t>::min();
    static constexpr auto max = std::numeric_limits<intmax_t>::max();

    static constexpr auto lmin = static_cast<typename T::value_type>(limits_lhs::min());
    static constexpr auto lmax = static_cast<typename T::value_type>(limits_lhs::max());

    static constexpr auto rmin = static_cast<typename U::value_type>(limits_rhs::min());
    static constexpr auto rmax = static_cast<typename U::value_type>(limits_rhs::max());

    // @@@TODO: VS complains with C4307 here (neither gcc nor clang complain - seems VS doesn't short-circuit here?)
    static_assert(true
      && ((lmax <= max) && (rmax <= max))
      && ((lmax < 0) || (rmax <= (max - lmax)))
      && ((lmin > 0) || (rmin >= (min - lmin)))
      , "SafeInt: sum result cannot be represented using native types");

    static constexpr intmax_t newmin = static_cast<intmax_t>(lmin) + static_cast<intmax_t>(rmin);
    static constexpr intmax_t newmax = static_cast<intmax_t>(lmax) + static_cast<intmax_t>(rmax);

    using result_type = safe_signed<newmin, newmax>;
    using value_type = typename result_type::value_type;

    static constexpr auto call(T const lhs, U const rhs) noexcept
    {
      return result_type{static_cast<value_type>(
          static_cast<value_type>(static_cast<typename T::value_type>(lhs))
        + static_cast<value_type>(static_cast<typename U::value_type>(rhs))
        ), unchecked_construct};
    }
  };

  template<typename T, typename U>
  struct add<T, U, false>
  {
    using limits_lhs = std::numeric_limits<T>;
    using limits_rhs = std::numeric_limits<U>;

    static constexpr auto max = std::numeric_limits<uintmax_t>::max();

    static constexpr auto lmin = static_cast<typename T::value_type>(limits_lhs::min());
    static constexpr auto lmax = static_cast<typename T::value_type>(limits_lhs::max());

    static constexpr auto rmin = static_cast<typename U::value_type>(limits_rhs::min());
    static constexpr auto rmax = static_cast<typename U::value_type>(limits_rhs::max());

    // @@@TODO: VS complains with C4307 here (neither gcc nor clang complain - seems VS doesn't short-circuit here?)
    static_assert(rmax <= (max - lmax)
      , "SafeInt: sum result cannot be represented using native types");

    static constexpr uintmax_t newmin = static_cast<uintmax_t>(lmin) + static_cast<uintmax_t>(rmin);
    static constexpr uintmax_t newmax = static_cast<uintmax_t>(lmax) + static_cast<uintmax_t>(rmax);

    using result_type = safe_unsigned<newmin, newmax>;
    using value_type = typename result_type::value_type;

    static constexpr auto call(T lhs, U rhs) noexcept
    {
      return result_type{static_cast<value_type>(
          static_cast<value_type>(static_cast<typename T::value_type>(lhs))
        + static_cast<value_type>(static_cast<typename U::value_type>(rhs))
        ), unchecked_construct};
    }
  };
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr auto operator+(T lhs, U rhs) noexcept
{
  return detail::add<T, U>::call(lhs, rhs);
}

// subtraction helpers
namespace detail
{
  template
  <
    typename T
  , typename U
  , bool = (std::is_signed_v<typename T::value_type> || std::is_signed_v<typename U::value_type> || (std::numeric_limits<T>::min() >= std::numeric_limits<U>::max()))
  >
  struct sub;

  template<typename T, typename U>
  struct sub<T, U, true>
  {
    using limits_lhs = std::numeric_limits<T>;
    using limits_rhs = std::numeric_limits<U>;

    static constexpr auto min = std::numeric_limits<intmax_t>::min();
    static constexpr auto max = std::numeric_limits<intmax_t>::max();

    static constexpr auto lmin = static_cast<typename T::value_type>(limits_lhs::min());
    static constexpr auto lmax = static_cast<typename T::value_type>(limits_lhs::max());

    static constexpr auto rmin = static_cast<typename U::value_type>(limits_rhs::min());
    static constexpr auto rmax = static_cast<typename U::value_type>(limits_rhs::max());

    // @@@TODO: VS complains with C4307 here (neither gcc nor clang complain - seems VS doesn't short-circuit here?)
    static_assert(true
      && ((lmax <= max) && (rmax <= max))
      && ((rmax < 0) || (lmin >= (min + rmax)))
      && ((rmin > 0) || (lmax <= (max + rmin)))
      , "SafeInt: sum result cannot be represented using native types");

    static constexpr intmax_t newmin = static_cast<intmax_t>(lmin) - static_cast<intmax_t>(rmax);
    static constexpr intmax_t newmax = static_cast<intmax_t>(lmax) - static_cast<intmax_t>(rmin);

    using result_type = safe_signed<newmin, newmax>;
    using value_type = typename result_type::value_type;

    static constexpr auto call(T lhs, U rhs) noexcept
    {
      return result_type{static_cast<value_type>(
          static_cast<value_type>(static_cast<typename T::value_type>(lhs))
        - static_cast<value_type>(static_cast<typename U::value_type>(rhs))
        ), unchecked_construct};
    }
  };

  template<typename T, typename U>
  struct sub<T, U, false>
  {
    using limits_lhs = std::numeric_limits<T>;
    using limits_rhs = std::numeric_limits<U>;

    static constexpr auto lmin = static_cast<typename T::value_type>(limits_lhs::min());
    static constexpr auto lmax = static_cast<typename T::value_type>(limits_lhs::max());

    static constexpr auto rmin = static_cast<typename U::value_type>(limits_rhs::min());
    static constexpr auto rmax = static_cast<typename U::value_type>(limits_rhs::max());

    // we only select this overload if lmin >= rmax, so the result cannot underflow
    // both types are unsigned, so the difference cannot overflow

    static constexpr uintmax_t newmin = static_cast<uintmax_t>(lmin) - static_cast<uintmax_t>(rmax);
    static constexpr uintmax_t newmax = static_cast<uintmax_t>(lmax) - static_cast<uintmax_t>(rmin);

    using result_type = safe_unsigned<newmin, newmax>;
    using value_type = typename result_type::value_type;

    static constexpr auto call(T lhs, U rhs) noexcept
    {
      return result_type{static_cast<value_type>(
          static_cast<value_type>(static_cast<typename T::value_type>(lhs))
        - static_cast<value_type>(static_cast<typename U::value_type>(rhs))
        ), unchecked_construct};
    }
  };
}

template
<
  typename T
, typename U
, typename = std::enable_if_t<(detail::is_safe_v<T> && detail::is_safe_v<U>)>
>
constexpr auto operator-(T lhs, U rhs) noexcept
{
  return detail::sub<T, U>::call(lhs, rhs);
}

template<typename T, T min, T max>
struct is_packable<safe<T, min, max>> : std::true_type
{
};

namespace detail
{
  // @@@FIXME: signed overflow when max-min >= intmax_t max
  template<typename T, T min, T max>
  struct signed_packable_traits
  {
    static constexpr uintmax_t packed_size = (min != max) ? (1U + log2_v<(static_cast<intmax_t>(max) - static_cast<intmax_t>(min))>) : 0U;
    using value_type = safe<T, min, max>;
    using packed_type = bitstream<packed_size>;

    static constexpr packed_type pack(value_type const &v)
    {
      return{static_cast<uintmax_t>(static_cast<intmax_t>(static_cast<T>(v)) - static_cast<intmax_t>(min))};
    }

    static constexpr value_type unpack(packed_type const &v)
    {
      return value_type{static_cast<T>(static_cast<intmax_t>(v.to_ullong()) + static_cast<intmax_t>(min)), unchecked_construct};
    }
  };

  template<typename T, T min, T max>
  struct unsigned_packable_traits
  {
    static constexpr uintmax_t packed_size = (min != max) ? (1U + log2_v<(max - min)>) : 0U;
    using value_type = safe<T, min, max>;
    using packed_type = bitstream<packed_size>;

    static constexpr packed_type pack(value_type const &v)
    {
      return{static_cast<T>(v) - min};
    }

    static constexpr value_type unpack(packed_type const &v)
    {
      return value_type{static_cast<T>(v.to_ullong()) + static_cast<T>(min), unchecked_construct};
    }
  };
}

template<typename T, T min, T max>
struct packable_traits<safe<T, min, max>>
  : std::conditional_t<std::is_signed_v<T>
  , detail::signed_packable_traits<T, min, max>
  , detail::unsigned_packable_traits<T, min, max>>
{
};

} // namespace rdk

namespace std
{

template<typename T, T minV, T maxV>
struct numeric_limits<::rdk::safe<T, minV, maxV>>
  : public std::numeric_limits<T>
{
  using value_type = ::rdk::safe<T, minV, maxV>;

  constexpr static bool traps = true;
  constexpr static bool is_bounded = true;
  constexpr static bool is_modulo = false;

  constexpr static value_type min() noexcept
  {
    return value_type{minV, ::rdk::unchecked_construct};
  }

  constexpr static value_type lowest() noexcept
  {
    return min();
  }

  constexpr static value_type max() noexcept
  {
    return value_type{maxV, ::rdk::unchecked_construct};
  }
};

} // namespace std

#endif // !RDK_962F9BEDF2C749EDAD5BFB085F2951F7
