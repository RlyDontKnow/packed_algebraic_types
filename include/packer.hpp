#pragma once
#ifndef RDK_03F2F1279F55C951054ADBCFDAE92797
#define RDK_03F2F1279F55C951054ADBCFDAE92797

#include <cstdint>
#include <bitset>
#include <type_traits>

namespace rdk
{

template<uintmax_t v>
struct log2 : std::integral_constant<uint64_t, (1U + log2<(v >> 1U)>::value)>
{
};

template<>
struct log2<0U> : std::integral_constant<uintmax_t, 0U>
{
};

template<>
struct log2<1U> : std::integral_constant<uintmax_t, 0U>
{
};

template<uintmax_t v>
constexpr uintmax_t log2_v = log2<v>::value;

template<typename T>
struct is_packable : std::false_type
{
};

template<typename T>
constexpr bool is_packable_v = is_packable<T>::value;

// @@@TODO: more suitable type
template<size_t bits>
using bitstream = std::bitset<bits>;

template<typename T>
struct packable_traits;



}

#endif // !RDK_03F2F1279F55C951054ADBCFDAE92797 
