/**
 * @file   type_traits.h
 * @author Dennis Sitelew 
 * @date   Feb 18, 2024
 */

#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace cbor {

template <typename T>
inline constexpr auto min_int_v = std::numeric_limits<T>::min();

template <typename T>
inline constexpr auto max_int_v = std::numeric_limits<T>::max();

template <typename T>
struct is_bool : std::bool_constant<false> {};

template <>
struct is_bool<bool> : std::bool_constant<true> {};

template <typename T>
using is_bool_t = is_bool<std::decay_t<T>>::type;

template <typename T>
inline constexpr bool is_bool_v = is_bool_t<T>::value;

template <typename T>
concept IsBool = is_bool_v<T>;

template <typename T>
concept UnsignedInt = std::is_unsigned_v<T> && std::is_integral_v<T> && !is_bool_v<T>;

template <typename T>
concept SignedInt = std::is_signed_v<T> && std::is_integral_v<T>;

template <typename T>
concept ValueOrConstReferenceU8 = std::is_same_v<T, const std::uint8_t &> || std::is_same_v<T, std::uint8_t>;

template <class T>
concept ConstByteArray = requires(const T &t) {
   { *std::cbegin(t) } -> ValueOrConstReferenceU8;
   { std::size(t) } -> std::convertible_to<std::size_t>;
};

template <class T>
concept ByteArray = requires(T &t) {
   { *std::begin(t) } -> std::same_as<std::uint8_t &>;
   { std::size(t) } -> std::convertible_to<std::size_t>;
};

} // namespace cbor